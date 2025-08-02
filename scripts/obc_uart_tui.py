#!/usr/bin/env python3
import threading
import sys
import tty
import termios
import select
import logging
from datetime import datetime
from typing import Optional, Dict, Any
from dataclasses import dataclass
from enum import Enum
import argparse
import json
import os

import serial
from rich.console import Console
from rich.layout import Layout
from rich.panel import Panel
from rich.live import Live
from rich.text import Text
from rich.table import Table

from pymodbus.server import StartSerialServer
from pymodbus.datastore import (
    ModbusSlaveContext,
    ModbusServerContext,
    ModbusSequentialDataBlock,
)
from pymodbus.device import ModbusDeviceIdentification


class AppState(Enum):
    STARTING = "starting"
    RUNNING = "running"
    STOPPING = "stopping"
    STOPPED = "stopped"


@dataclass
class Config:
    uart_port: str = "/dev/ttyACM0"
    uart_baudrate: int = 115200
    modbus_port: str = "/dev/ttyACM1"
    modbus_baudrate: int = 115200
    max_uart_messages: int = 100
    max_log_entries: int = 50
    refresh_rate: int = 20
    log_file: Optional[str] = None

    @classmethod
    def from_file(cls, config_file: str) -> "Config":
        """Load configuration from JSON file."""
        try:
            with open(config_file, "r") as f:
                data = json.load(f)
            return cls(**data)
        except (FileNotFoundError, json.JSONDecodeError) as e:
            logging.warning(f"Could not load config file {config_file}: {e}")
            return cls()

    def to_file(self, config_file: str):
        """Save configuration to JSON file."""
        with open(config_file, "w") as f:
            json.dump(self.__dict__, f, indent=2)


# Filling State Machine Commands
CMD_GLOBAL_START = 0x00000001
CMD_IDLE_START = 0x00000100
CMD_OTHER_START = 0x00010000

CMD_MAP = {
    "STOP": CMD_GLOBAL_START,
    "ABORT": CMD_GLOBAL_START + 1,
    "PAUSE": CMD_GLOBAL_START + 2,
    "FILL_COPV": CMD_IDLE_START,
    "PRE_PRESSURIZE": CMD_IDLE_START + 1,
    "FILL_N20": CMD_IDLE_START + 2,
    "POST_PRESSURIZE": CMD_IDLE_START + 3,
    "READY": CMD_OTHER_START,
    "RESUME": CMD_OTHER_START + 1,
}


class ModbusSlaveSimulator:
    def __init__(self, port: str, baudrate: int):
        self.port = port
        self.baudrate = baudrate
        self.running = threading.Event()
        self.thread: Optional[threading.Thread] = None
        self.messages: list[str] = []
        self.connection_status = "Disconnected"

        # Initialize UF Hydra (Slave ID 1)
        self.uf_hydra_store = ModbusSlaveContext(
            di=None,
            co=ModbusSequentialDataBlock(0, [False] * 2),
            hr=None,
            ir=ModbusSequentialDataBlock(0, [0]),
        )

        # Initialize LF Hydra (Slave ID 2)
        self.lf_hydra_store = ModbusSlaveContext(
            di=None,
            co=ModbusSequentialDataBlock(0, [False] * 2),
            hr=None,
            ir=ModbusSequentialDataBlock(0, [0] * 3),
        )

        # Create context with multiple slaves
        self.context = ModbusServerContext(
            slaves={
                1: self.uf_hydra_store,  # UF Hydra on address 1
                2: self.lf_hydra_store,  # LF Hydra on address 2
            },
            single=False,
        )

    def start(self) -> bool:
        """Start the Modbus server."""
        try:
            self.running.set()
            self.thread = threading.Thread(target=self._run_server, daemon=True)
            self.thread.start()
            self.connection_status = "Connected"
            self.add_message(
                f"Modbus RTU slave started on {self.port} at {self.baudrate} baud"
            )
            return True
        except Exception as e:
            self.connection_status = "Error"
            self.add_message(f"Error starting Modbus server: {e}")
            logging.error(f"Modbus start error: {e}")
            return False

    def stop(self):
        """Stop the Modbus server."""
        self.running.clear()
        self.connection_status = "Disconnected"
        if self.thread and self.thread.is_alive():
            self.thread.join(timeout=2)

    def add_message(self, message: str):
        """Add a timestamped message to the message queue."""
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        self.messages.append(f"[{timestamp}] MODBUS: {message}")

    def handle_hr(self, slave_id: int, index: int, value: int):
        """Handle setting a holding register value for a specific slave."""
        try:
            if slave_id in self.context:
                slave_context = self.context[slave_id]
                if slave_context.store["h"] is not None:  # Check if HR exists
                    slave_context.setValues(3, index, [value])
                    self.add_message(
                        f"Slave {slave_id}: Holding register {index} set to {value}"
                    )
                else:
                    self.add_message(
                        f"Slave {slave_id}: No holding registers available"
                    )
            else:
                self.add_message(f"Invalid slave ID: {slave_id}")
        except Exception as e:
            self.add_message(f"Error setting HR {index} on slave {slave_id}: {e}")

    def handle_coil(self, slave_id: int, index: int, value: bool):
        """Handle setting a coil value for a specific slave."""
        try:
            if slave_id in self.context:
                slave_context = self.context[slave_id]
                if slave_context.store["c"] is not None:  # Check if coils exist
                    slave_context.setValues(1, index, [bool(value)])
                    self.add_message(
                        f"Slave {slave_id}: Coil {index} set to {bool(value)}"
                    )
                else:
                    self.add_message(f"Slave {slave_id}: No coils available")
            else:
                self.add_message(f"Invalid slave ID: {slave_id}")
        except Exception as e:
            self.add_message(f"Error setting coil {index} on slave {slave_id}: {e}")

    def handle_input_register(self, slave_id: int, index: int, value: int):
        """Handle setting an input register value for a specific slave."""
        try:
            if slave_id in self.context:
                slave_context = self.context[slave_id]
                if slave_context.store["i"] is not None:  # Check if IR exists
                    slave_context.setValues(4, index, [value])
                    self.add_message(
                        f"Slave {slave_id}: Input register {index} set to {value}"
                    )
                else:
                    self.add_message(f"Slave {slave_id}: No input registers available")
            else:
                self.add_message(f"Invalid slave ID: {slave_id}")
        except Exception as e:
            self.add_message(f"Error setting IR {index} on slave {slave_id}: {e}")

    def get_register_values(
        self, slave_id: int, start: int = 0, count: int = 10
    ) -> Dict[str, Any]:
        """Get current register values for display from a specific slave."""
        try:
            if slave_id in self.context:
                slave_context = self.context[slave_id]
                values = {}

                # Try to get holding registers
                try:
                    if slave_context.store["h"] is not None:
                        hr_values = slave_context.getValues(3, start, min(count, 10))
                        for i, val in enumerate(hr_values):
                            values[f"HR{start + i}"] = val
                except Exception as e:
                    self.add_message(f"Error reading HR: {e}")

                # Try to get input registers
                try:
                    if slave_context.store["i"] is not None:
                        ir_values = slave_context.getValues(4, start, min(count, 10))
                        for i, val in enumerate(ir_values):
                            values[f"IR{start + i}"] = val
                except Exception as e:
                    self.add_message(f"Error reading IR: {e}")

                # Try to get coils
                try:
                    if slave_context.store["c"] is not None:
                        coil_values = slave_context.getValues(1, start, min(count, 10))
                        for i, val in enumerate(coil_values):
                            values[f"C{start + i}"] = int(val)
                except Exception as e:
                    self.add_message(f"Error reading coils: {e}")

                return values
            return {}
        except Exception:
            return {}

    def get_slave_info(self) -> Dict[int, Dict[str, Any]]:
        """Get information about all configured slaves."""
        info = {}
        for slave_id in [1, 2]:  # UF and LF Hydra
            if slave_id in self.context:
                slave_context = self.context[slave_id]
                slave_name = "UF Hydra" if slave_id == 1 else "LF Hydra"

                info[slave_id] = {
                    "name": slave_name,
                    "coils": slave_context.store["c"] is not None,
                    "discrete_inputs": slave_context.store["d"] is not None,
                    "holding_registers": slave_context.store["h"] is not None,
                    "input_registers": slave_context.store["i"] is not None,
                }

                # Get counts if available
                if slave_context.store["c"] is not None:
                    info[slave_id]["coil_count"] = len(slave_context.store["c"].values)
                if slave_context.store["i"] is not None:
                    info[slave_id]["ir_count"] = len(slave_context.store["i"].values)

        return info

    def _run_server(self):
        """Run the Modbus server."""
        try:
            identity = ModbusDeviceIdentification()
            identity.VendorName = "Pymodbus"
            identity.ProductCode = "PM"
            identity.VendorUrl = "http://github.com/riptideio/pymodbus/"
            identity.ProductName = "Modbus RTU Slave Simulator"
            identity.ModelName = "Modbus RTU Slave"
            identity.MajorMinorRevision = "1.0"

            StartSerialServer(
                context=self.context,
                identity=identity,
                port=self.port,
                baudrate=self.baudrate,
                stopbits=1,
                bytesize=8,
                parity="N",
            )
        except Exception as e:
            self.connection_status = "Error"
            self.add_message(f"Server error: {e}")
            logging.error(f"Modbus server error: {e}")


class UARTListener:
    def __init__(self, port: str, baudrate: int, max_messages: int = 100):
        self.port = port
        self.baudrate = baudrate
        self.serial_connection: Optional[serial.Serial] = None
        self.running = threading.Event()
        self.thread: Optional[threading.Thread] = None
        self.messages: list[str] = []
        self.max_messages = max_messages
        self.connection_status = "Disconnected"
        self.bytes_sent = 0
        self.bytes_received = 0

    def start(self) -> bool:
        """Start the UART listener."""
        try:
            self.serial_connection = serial.Serial(
                self.port, self.baudrate, timeout=1, write_timeout=1
            )
            self.running.set()
            self.thread = threading.Thread(target=self._listen, daemon=True)
            self.thread.start()
            self.connection_status = "Connected"
            self.add_message(f"UART connected to {self.port} at {self.baudrate} baud")
            return True
        except Exception as e:
            self.connection_status = "Error"
            self.add_message(f"UART connection error: {e}")
            logging.error(f"UART start error: {e}")
            return False

    def stop(self):
        """Stop the UART listener."""
        self.running.clear()
        self.connection_status = "Disconnected"
        if self.thread and self.thread.is_alive():
            self.thread.join(timeout=2)
        if self.serial_connection and self.serial_connection.is_open:
            self.serial_connection.close()

    def add_message(self, message: str):
        """Add a timestamped message to the message queue."""
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        self.messages.append(f"[{timestamp}] {message}")
        if len(self.messages) > self.max_messages:
            self.messages.pop(0)

    def _listen(self):
        """Listen for incoming UART data."""
        while self.running.is_set():
            try:
                if self.serial_connection and self.serial_connection.in_waiting:
                    data = (
                        self.serial_connection.readline()
                        .decode("utf-8", errors="ignore")
                        .strip()
                    )
                    if data:
                        self.bytes_received += len(data)
                        self.add_message(f"RX: {data}")
            except Exception as e:
                self.connection_status = "Error"
                self.add_message(f"UART Error: {e}")
                logging.error(f"UART listen error: {e}")
                break

    def send(self, command: str) -> bool:
        """Send a command via UART."""
        try:
            if self.serial_connection and self.serial_connection.is_open:
                message = command + "\n"
                self.serial_connection.write(message.encode())
                self.bytes_sent += len(message)
                self.add_message(f"TX: {command}")
                return True
        except Exception as e:
            self.add_message(f"Send Error: {e}")
            logging.error(f"UART send error: {e}")
        return False


class CommandProcessor:
    def __init__(
        self, uart_listener: UARTListener, modbus_simulator: ModbusSlaveSimulator
    ):
        self.uart_listener = uart_listener
        self.modbus_simulator = modbus_simulator
        self.command_history: list[str] = []
        self.max_history = 100

    def process_command(self, command: str) -> list[str]:
        """Process a command and return response messages."""
        if not command.strip():
            return []

        self.command_history.append(command)
        if len(self.command_history) > self.max_history:
            self.command_history.pop(0)

        parts = command.strip().split()
        cmd = parts[0].lower()
        responses = [f"> {command}"]

        if cmd == "help":
            responses.extend(
                [
                    "Available commands:",
                    "  help - Show this help",
                    "  uart <message> - Send message to UART",
                    "  cmd <command> - Send filling state machine command",
                    f"    Available: {', '.join(CMD_MAP.keys())}",
                    "  modbus <slave_id> <hr|coil|ir> <index> <value> - Set Modbus register/coil",
                    "    slave_id: 1 (UF Hydra) or 2 (LF Hydra)",
                    "  status - Show connection status and statistics",
                    "  history - Show command history",
                    "  clear - Clear command log",
                    "  save-config [file] - Save current config",
                    "  quit/exit - Exit application",
                ]
            )
        elif cmd == "uart":
            if len(parts) > 1:
                message = " ".join(parts[1:])
                if self.uart_listener.send(message):
                    responses.append(f"✓ Sent: {message}")
                else:
                    responses.append("✗ Failed to send UART command")
            else:
                responses.append("Usage: uart <message>")
        elif cmd == "cmd":
            if len(parts) > 1:
                command_name = parts[1].upper()
                if command_name in CMD_MAP:
                    command_value = CMD_MAP[command_name]
                    payload = f"normal 0x{command_value:08X}"
                    if self.uart_listener.send(payload):
                        responses.append(f"✓ Sent command: {command_name} ({payload})")
                    else:
                        responses.append("✗ Failed to send command")
                else:
                    responses.append(f"✗ Unknown command: {command_name}")
                    responses.append(f"Available: {', '.join(CMD_MAP.keys())}")
            else:
                responses.append("Usage: cmd <command>")
        elif cmd == "modbus":
            if len(parts) < 5:
                responses.append(
                    "Usage: modbus <slave_id> <hr|coil|ir> <index> <value>"
                )
                responses.append("  slave_id: 1 (UF Hydra) or 2 (LF Hydra)")
                responses.append(
                    "  hr = holding register, coil = coil, ir = input register"
                )
            else:
                try:
                    slave_id = int(parts[1])
                    modbus_type = parts[2].lower()
                    index = int(parts[3])

                    if modbus_type == "hr":
                        value = int(parts[4])
                        self.modbus_simulator.handle_hr(slave_id, index, value)
                        responses.append(
                            f"✓ Set slave {slave_id} holding register {index} = {value}"
                        )
                    elif modbus_type == "coil":
                        value = parts[4].lower() in ("1", "true", "yes", "on")
                        self.modbus_simulator.handle_coil(slave_id, index, value)
                        responses.append(
                            f"✓ Set slave {slave_id} coil {index} = {value}"
                        )
                    elif modbus_type == "ir":
                        value = int(parts[4])
                        self.modbus_simulator.handle_input_register(
                            slave_id, index, value
                        )
                        responses.append(
                            f"✓ Set slave {slave_id} input register {index} = {value}"
                        )
                    else:
                        responses.append(f"✗ Unknown Modbus type: {modbus_type}")
                        responses.append("Valid types: hr, coil, ir")
                except ValueError as e:
                    responses.append(f"✗ Invalid value: {e}")
                except Exception as e:
                    responses.append(f"✗ Error: {e}")
        elif cmd == "status":
            responses.extend(
                [
                    f"UART: {self.uart_listener.connection_status} ({self.uart_listener.port})",
                    f"  TX: {self.uart_listener.bytes_sent} bytes, RX: {self.uart_listener.bytes_received} bytes",
                    f"Modbus: {self.modbus_simulator.connection_status} ({self.modbus_simulator.port})",
                    f"Messages: UART={len(self.uart_listener.messages)}, Modbus={len(self.modbus_simulator.messages)}",
                ]
            )
        elif cmd == "history":
            responses.append("Command history:")
            for i, hist_cmd in enumerate(self.command_history[-10:], 1):
                responses.append(f"  {i:2d}. {hist_cmd}")
        elif cmd == "clear":
            return ["Command log cleared"]  # Special case - caller should clear log
        elif cmd == "save-config":
            config_file = parts[1] if len(parts) > 1 else "config.json"
            try:
                # This would need access to the current config
                responses.append(
                    f"Config save feature needs implementation for {config_file}"
                )
            except Exception as e:
                responses.append(f"✗ Failed to save config: {e}")
        elif cmd in ("quit", "exit"):
            responses.append("Shutting down...")
            return responses + ["QUIT"]  # Special marker for main loop
        else:
            responses.append(f"✗ Unknown command: {command}")
            responses.append("Type 'help' for available commands")

        return responses


class RichTerminalApp:
    def __init__(self, config: Config):
        self.config = config
        self.console = Console()
        self.uart_listener = UARTListener(
            config.uart_port, config.uart_baudrate, config.max_uart_messages
        )
        self.modbus_simulator = ModbusSlaveSimulator(
            config.modbus_port, config.modbus_baudrate
        )
        self.command_processor = CommandProcessor(
            self.uart_listener, self.modbus_simulator
        )
        self.command_log: list[str] = []
        self.user_input = ""
        self.state = AppState.STARTING
        self.running = True

        # Setup logging
        if config.log_file:
            logging.basicConfig(
                level=logging.INFO,
                format="%(asctime)s - %(levelname)s - %(message)s",
                handlers=[
                    logging.FileHandler(config.log_file),
                    logging.StreamHandler(sys.stderr),
                ],
            )

        self._setup_layout()

    def _setup_layout(self):
        """Setup the Rich layout."""
        self.layout = Layout()
        self.layout.split_row(
            Layout(name="left", ratio=1), Layout(name="right", ratio=1)
        )
        self.layout["left"].split_column(
            Layout(name="command_log", ratio=3),
            Layout(name="command_input", size=3),
            Layout(name="status", size=4),
        )
        self.layout["right"].split_column(
            Layout(name="uart_output", ratio=2), Layout(name="modbus_status", ratio=1)
        )

    def log_command(self, message: str):
        """Add a message to the command log."""
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.command_log.append(f"[{timestamp}] {message}")
        if len(self.command_log) > self.config.max_log_entries:
            self.command_log.pop(0)

    def update_display(self):
        """Update the display with current data."""
        # Process new messages from subsystems
        for message in self.modbus_simulator.messages:
            self.log_command(message)
        self.modbus_simulator.messages.clear()

        # Calculate visible lines for each panel
        term_height = self.console.size.height
        log_lines = max(int((term_height - 10) * 0.4), 10)
        uart_lines = max(int((term_height - 6) * 0.6), 15)

        # Command log panel
        command_text = Text()
        for line in self.command_log[-log_lines:]:
            # Color-code different message types
            if "✓" in line:
                command_text.append(line + "\n", style="green")
            elif "✗" in line:
                command_text.append(line + "\n", style="red")
            elif "MODBUS:" in line:
                command_text.append(line + "\n", style="blue")
            else:
                command_text.append(line + "\n")

        self.layout["command_log"].update(
            Panel(command_text, title="Command Log", border_style="blue")
        )

        # Command input panel
        prompt = f"[{self.state.value}] >>> "
        self.layout["command_input"].update(
            Panel(
                prompt + self.user_input + "█",
                title="Command Input",
                border_style="green",
            )
        )

        # Status panel
        status_table = Table.grid(padding=(0, 1))
        status_table.add_column(style="cyan", no_wrap=True)
        status_table.add_column(style="white")
        status_table.add_row(
            "UART:",
            f"{self.uart_listener.connection_status} ({self.uart_listener.port})",
        )
        status_table.add_row(
            "Modbus:",
            f"{self.modbus_simulator.connection_status} ({self.modbus_simulator.port})",
        )
        status_table.add_row("State:", self.state.value.title())

        self.layout["status"].update(
            Panel(status_table, title="System Status", border_style="yellow")
        )

        # UART output panel
        uart_text = Text(overflow="ellipsis", no_wrap=False)
        for line in self.uart_listener.messages[-uart_lines:]:
            if "TX:" in line:
                uart_text.append(line + "\n", style="green")
            elif "RX:" in line:
                uart_text.append(line + "\n", style="cyan")
            elif "Error" in line:
                uart_text.append(line + "\n", style="red")
            else:
                uart_text.append(line + "\n")

        self.layout["uart_output"].update(
            Panel(
                uart_text,
                title="UART Communication",
                border_style="red",
                padding=(0, 1),
            )
        )

        # Modbus status panel - show both slaves
        modbus_table = Table("Slave", "Register", "Value", title="Modbus Status")
        modbus_table.add_column("Slave", style="yellow", no_wrap=True)
        modbus_table.add_column("Register", style="cyan", no_wrap=True)
        modbus_table.add_column("Value", style="white")

        # Get slave info
        slave_info = self.modbus_simulator.get_slave_info()

        for slave_id in [1, 2]:
            if slave_id in slave_info:
                slave_name = slave_info[slave_id]["name"]
                registers = self.modbus_simulator.get_register_values(slave_id, 0, 5)

                if not registers:
                    modbus_table.add_row(slave_name, "No data", "-")
                else:
                    first_row = True
                    for reg_name, value in registers.items():
                        display_name = slave_name if first_row else ""
                        modbus_table.add_row(display_name, reg_name, str(value))
                        first_row = False

        self.layout["modbus_status"].update(
            Panel(modbus_table, title="Modbus Status", border_style="magenta")
        )

    def process_command(self, command: str):
        """Process a command and update the log."""
        responses = self.command_processor.process_command(command)

        if "QUIT" in responses:
            self.running = False
            responses.remove("QUIT")

        if "Command log cleared" in responses:
            self.command_log.clear()
            responses.remove("Command log cleared")
            self.log_command("Command log cleared")

        for response in responses:
            self.log_command(response)

    def getch(self, timeout=0.05):
        """Get a character from stdin with timeout."""
        dr, _, _ = select.select([sys.stdin], [], [], timeout)
        if dr:
            return sys.stdin.read(1)
        return None

    def start(self):
        """Start the application."""
        self.state = AppState.STARTING

        # Start subsystems
        uart_started = self.uart_listener.start()
        modbus_started = self.modbus_simulator.start()

        if uart_started:
            self.log_command("✓ UART listener started")
        else:
            self.log_command("✗ Failed to start UART listener")

        if modbus_started:
            self.log_command("✓ Modbus simulator started")
        else:
            self.log_command("✗ Failed to start Modbus simulator")

        self.state = AppState.RUNNING
        self.log_command("Application ready. Type 'help' for commands.")

        # Terminal setup
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)

        try:
            tty.setcbreak(fd)
            with Live(
                self.layout,
                console=self.console,
                refresh_per_second=self.config.refresh_rate,
                screen=True,
            ):
                while self.running:
                    c = self.getch()
                    if c is not None:
                        if c in ("\n", "\r"):
                            cmd = self.user_input
                            self.user_input = ""
                            if cmd.strip():
                                self.process_command(cmd)
                        elif c == "\x7f":  # Backspace
                            self.user_input = self.user_input[:-1]
                        elif c == "\x03":  # Ctrl+C
                            self.running = False
                            break
                        elif c == "\x09":  # Tab - command completion
                            # Simple command completion
                            if self.user_input.strip():
                                commands = [
                                    "help",
                                    "uart ",
                                    "cmd ",
                                    "modbus ",
                                    "status",
                                    "clear",
                                    "quit",
                                ]
                                matches = [
                                    cmd
                                    for cmd in commands
                                    if cmd.startswith(self.user_input)
                                ]
                                if len(matches) == 1:
                                    self.user_input = matches[0]
                        elif c.isprintable():
                            self.user_input += c

                    self.update_display()

        except KeyboardInterrupt:
            self.running = False
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
            self.cleanup()

    def cleanup(self):
        """Clean up resources."""
        self.state = AppState.STOPPING
        self.log_command("Shutting down...")
        self.uart_listener.stop()
        self.modbus_simulator.stop()
        self.state = AppState.STOPPED


def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(description="UART/Modbus Terminal Application")
    parser.add_argument("--config", "-c", help="Configuration file path")
    parser.add_argument("--uart-port", help="UART port path")
    parser.add_argument("--uart-baud", type=int, help="UART baud rate")
    parser.add_argument("--modbus-port", help="Modbus port path")
    parser.add_argument("--modbus-baud", type=int, help="Modbus baud rate")
    parser.add_argument("--log-file", help="Log file path")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose logging")
    return parser.parse_args()


def main():
    """Main entry point."""
    args = parse_arguments()

    # Load configuration
    if args.config and os.path.exists(args.config):
        config = Config.from_file(args.config)
    else:
        config = Config()

    # Override config with command line arguments
    if args.uart_port:
        config.uart_port = args.uart_port
    if args.uart_baud:
        config.uart_baudrate = args.uart_baud
    if args.modbus_port:
        config.modbus_port = args.modbus_port
    if args.modbus_baud:
        config.modbus_baudrate = args.modbus_baud
    if args.log_file:
        config.log_file = args.log_file

    if args.verbose:
        logging.basicConfig(level=logging.DEBUG)

    # Create and start application
    app = RichTerminalApp(config)
    try:
        app.start()
    except KeyboardInterrupt:
        print("\nApplication interrupted by user")
    except Exception as e:
        print(f"Application error: {e}")
        logging.error(f"Application error: {e}", exc_info=True)


if __name__ == "__main__":
    main()
