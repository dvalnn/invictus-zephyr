#!/usr/bin/env python3
import threading
import sys
import tty
import termios
import select
from datetime import datetime
from typing import Optional

import serial
from rich.console import Console
from rich.layout import Layout
from rich.panel import Panel
from rich.live import Live
from rich.text import Text


from pymodbus.server import StartSerialServer
from pymodbus.datastore import (
    ModbusSlaveContext,
    ModbusServerContext,
    ModbusSequentialDataBlock,
)
from pymodbus.device import ModbusDeviceIdentification


# Filling State Machine Commands
CMD_GLOBAL_START = 0x00000001
CMD_IDLE_START = 0x00000100
CMD_OTHER_START = 0x00010000

cmd_map = {
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


# Modbus data store
class ModbusSlaveSimulator:
    def __init__(self, port: str, baudrate: int):
        self.port = port
        self.baudrate = baudrate
        self.running = threading.Event()
        self.thread: Optional[threading.Thread] = None
        self.messages: list[str] = []
        self.store = ModbusSlaveContext(
            di=ModbusSequentialDataBlock(0, [0] * 100),
            co=ModbusSequentialDataBlock(0, [False] * 100),
            hr=ModbusSequentialDataBlock(0, [0] * 100),
            ir=ModbusSequentialDataBlock(0, [0] * 100),
        )
        self.context = ModbusServerContext(slaves=self.store, single=True)

    def start(self):
        try:
            self.running.set()
            self.thread = threading.Thread(target=self._run_server, daemon=True)
            self.thread.start()
            self.messages.append(
                f"Modbus RTU slave simulator started on {self.port} at {self.baudrate} baud."
            )
            return True
        except Exception as e:
            self.messages.append(f"Error starting Modbus server: {e}")
            return False

    def stop(self):
        self.running.clear()
        if self.thread and self.thread.is_alive():
            self.thread.join(timeout=1)

    def handle_hr(self, index: int, value: int):
        """Handle setting a holding register value."""
        self.context[0].setValues(3, index, [value])
        self.messages.append(f"Holding register {index} set to {value}")

    def handle_coil(self, index: int, value: bool):
        """Handle setting a coil value."""
        self.context[0].setValues(1, index, [bool(value)])
        self.messages.append(f"Coil {index} set to {bool(value)}")

    def _run_server(self):
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


class SimpleUARTListener:
    def __init__(self, port: str, baudrate: int):
        self.port = port
        self.baudrate = baudrate
        self.serial_connection: Optional[serial.Serial] = None
        self.running = threading.Event()
        self.thread: Optional[threading.Thread] = None
        self.messages: list[str] = []
        self.max_messages = 100

    def start(self):
        try:
            self.serial_connection = serial.Serial(self.port, self.baudrate, timeout=1)
            self.running.set()
            self.thread = threading.Thread(target=self._listen, daemon=True)
            self.thread.start()
            return True
        except Exception as e:
            self.messages.append(f"Error: {e}")
            return False

    def stop(self):
        self.running.clear()
        if self.thread and self.thread.is_alive():
            self.thread.join(timeout=1)
        if self.serial_connection:
            self.serial_connection.close()

    def _listen(self):
        while self.running.is_set():
            try:
                if self.serial_connection and self.serial_connection.in_waiting:
                    data = (
                        self.serial_connection.readline()
                        .decode("utf-8", errors="ignore")
                        .strip()
                    )
                    if data:
                        timestamp = datetime.now().strftime("%H:%M:%S")
                        self.messages.append(f"[{timestamp}] {data}")
                        if len(self.messages) > self.max_messages:
                            self.messages.pop(0)

            except Exception as e:
                self.messages.append(f"UART Error: {e}")
                break

    def send(self, command: str):
        try:
            if self.serial_connection and self.serial_connection.is_open:
                self.serial_connection.write((command + "\n").encode())
                return True
        except Exception as e:
            self.messages.append(f"Send Error: {e}")
        return False


class RichTwoColumnApp:
    def __init__(
        self,
        uart_port: str = "/dev/ttyACM0",
        uart_baudrate: int = 115200,
        modbus_port: str = "/dev/ttyACM1",
        modbus_baudrate: int = 115200,
    ):
        self.console = Console()
        self.uart_listener = SimpleUARTListener(uart_port, uart_baudrate)
        self.modbus_simulator = ModbusSlaveSimulator(modbus_port, modbus_baudrate)
        self.command_log: list[str] = []
        self.max_log_entries = 50
        self.running = True
        self.user_input = ""

        self.layout = Layout()
        self.layout.split_row(
            Layout(name="left", ratio=1), Layout(name="right", ratio=1)
        )
        self.layout["left"].split_column(
            Layout(name="command_log", ratio=4), Layout(name="command_input", size=3)
        )

    def log_command(self, message: str):
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.command_log.append(f"[{timestamp}] {message}")
        if len(self.command_log) > self.max_log_entries:
            self.command_log.pop(0)

    def calc_panel_lines(self, panel_name):
        # This assumes vertical split: [command_log, command_input]
        term_height = self.console.size.height
        left_ratios = self.layout["left"].children  # [command_log, command_input]
        total_ratio = sum([c.ratio if c.ratio else 0 for c in left_ratios])

        # For command_log
        if panel_name == "command_log":
            log_panel_ratio = self.layout["left"]["command_log"].ratio or 0
            # Estimate lines: subtract ~4 lines for panel border & title, divvy by ratio
            visible = int((term_height - 2) * (log_panel_ratio / total_ratio)) - 4
            return max(visible, 20)
        # For UART output (right panel)
        if panel_name == "right":
            right_ratio = self.layout["right"].ratio or 0  # default half
            visible = int((term_height - 2) * (right_ratio / (total_ratio))) - 4
            # self.log_command(f"visible: {visible}, max: {max(visible, 20)}")
            return max(visible, 20)

        return 20

    def update_display(self):

        log_lines = self.calc_panel_lines("command_log")
        uart_lines = self.calc_panel_lines("right")

        # Command log (left)
        command_text = Text()

        for line in self.modbus_simulator.messages:
            self.log_command(line)

        self.modbus_simulator.messages = []

        for line in self.command_log[-log_lines:]:
            command_text.append(line + "\n")

        self.layout["command_log"].update(
            Panel(command_text, title="Command Shell & Logs", border_style="blue")
        )
        # Current live input (left-bottom)
        fake_prompt = ">>> "
        self.layout["command_input"].update(
            Panel(
                fake_prompt + self.user_input,
                title="Command Input (live)",
                border_style="green",
            )
        )
        # UART output (right)
        uart_text = Text(justify="full", no_wrap=True, overflow="fold", end="")
        for line in self.uart_listener.messages[-uart_lines:]:
            uart_text.append(line.strip() + "\n")
        self.layout["right"].update(
            Panel(uart_text, title="UART Output", border_style="red", padding=(1, 2))
        )

    def process_command(self, command: str):
        parts = command.strip().split()
        if not parts:
            return
        cmd = parts[0].lower()
        self.log_command(f"> {command}")

        if cmd == "help":
            help_lines = [
                "Available commands:",
                "  help - Show this help",
                "  uart <message> - Send message to UART (verbatim)",
                "  cmd <command> - Send filling state machine command.",
                "  \t-> available commands: " + ", ".join(cmd_map.keys()),
                "  modbus <hr|coil> <index> <value> - Set Modbus register/coil",
                "  clear - Clear command log",
                "  status - UART running/connected status",
                "  quit/exit - Exit application",
            ]
            for line in help_lines:
                self.log_command(line)
        elif cmd == "uart":
            if len(parts) > 1:
                message = " ".join(parts[1:])
                if self.uart_listener.send(message):
                    self.log_command(f"Sent: {message}")
                else:
                    self.log_command("Failed to send UART command")
            else:
                self.log_command("Usage: uart <message>")
        elif cmd == "cmd":
            if len(parts) > 1:
                command_name = parts[1].upper()
                if command_name in cmd_map:
                    command_value = cmd_map[command_name]
                    payload = f"normal 0x{command_value:08X}\n"
                    if self.uart_listener.send(payload):
                        self.log_command(
                            f"Sent command: {command_name} ({payload.strip()})"
                        )
                    else:
                        self.log_command("Failed to send command")
                else:
                    self.log_command(f"Unknown command: {command_name}")
            else:
                self.log_command("Usage: cmd <command>")

        elif cmd == "modbus":
            if len(parts) < 4:
                self.log_command("Usage: modbus <hr|coil> <index> <value>")
                return
            modbus_type = parts[1].lower()
            index = int(parts[2])
            if modbus_type == "hr":
                value = int(parts[3])
                self.modbus_simulator.handle_hr(index, value)
            elif modbus_type == "coil":
                value = parts[3].lower() in ("1", "true", "yes")
                self.modbus_simulator.handle_coil(index, value)
            else:
                self.log_command(f"Unknown Modbus type: {modbus_type}")

        elif cmd == "clear":
            self.command_log.clear()
            self.log_command("Command log cleared")
        elif cmd == "status":
            status = f"UART: {'Connected' if self.uart_listener.running.is_set() else 'Disconnected'}"
            self.log_command(status)
        elif cmd in ("quit", "exit"):
            self.running = False
        else:
            self.log_command(f"Unknown command: {command}")

    def getch(self, timeout=0.02):
        dr, _, _ = select.select([sys.stdin], [], [], timeout)
        if dr:
            return sys.stdin.read(1)
        return None

    def start(self):
        if self.uart_listener.start():
            self.log_command("UART listener started")
        else:
            self.log_command("Failed to start UART listener")

        if self.modbus_simulator.start():
            self.log_command("Modbus simulator started")
        else:
            self.log_command("Failed to start Modbus simulator")

        self.log_command("Commands: help, uart <msg>, clear, status, quit")
        self.log_command("Application ready. Type 'help' for commands.")

        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setcbreak(fd)
            with Live(
                self.layout, console=self.console, refresh_per_second=20, screen=True
            ):
                while self.running:
                    c = self.getch(timeout=0.02)
                    if c is not None:
                        if c in ("\n", "\r"):
                            cmd = self.user_input
                            self.user_input = ""
                            self.process_command(cmd)
                        elif c == "\x7f":  # Backspace
                            self.user_input = self.user_input[:-1]
                        elif c == "\x03":  # Ctrl+C
                            self.running = False
                            break
                        elif c.isprintable():
                            self.user_input += c
                    self.update_display()
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
            self.cleanup()

    def cleanup(self):
        self.uart_listener.stop()
        self.log_command("Application shutting down...")


def main():
    app = RichTwoColumnApp("/dev/ttyACM0", 115200)
    try:
        app.start()
    except KeyboardInterrupt:
        print("\nApplication interrupted by user")
    except Exception as e:
        print(f"Application error: {e}")


if __name__ == "__main__":
    main()
