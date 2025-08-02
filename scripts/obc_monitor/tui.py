import re
import sys
import tty
import termios
import select
import logging
from datetime import datetime
from enum import Enum

from rich.console import Console
from rich.layout import Layout
from rich.panel import Panel
from rich.live import Live
from rich.text import Text
from rich.table import Table

from .config import Config
from .commands import CommandProcessor
from .uart import UARTListener
from .modbus import ModbusSlaveSimulator

ansi_delete = re.compile(r"\x1B\[[0-?]*[ -/]*[@-~]")


class AppState(Enum):
    STARTING = "starting"
    RUNNING = "running"
    STOPPING = "stopping"
    STOPPED = "stopped"


class OBCMonitorTUI:
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
        uart_lines = max(int((term_height - 8) * 0.6), 15)

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
        uart_text = Text()
        for line in self.uart_listener.messages[-uart_lines:]:
            line = ansi_delete.sub("", line)  # Remove ANSI escape codes
            # split after "RX:" or "TX:" for color coding

            style = "green" if "RX:" in line else "cyan" if "TX:" in line else "white"
            parts = line.split("RX:", 1) if "RX:" in line else line.split("TX:", 1)
            text_style = (
                "yellow" if "<wrn>" in line else "red" if "<err>" in line else "white"
            )
            if len(parts) > 1:
                prefix, content = parts[0], parts[1]
                uart_text.append(prefix, style)
                uart_text.append(content.strip() + "\n", style=text_style)

        self.layout["uart_output"].update(
            Panel(
                uart_text,
                title="UART Communication",
                border_style="red",
            )
        )

        # Modbus status panel - show both slaves
        modbus_table = Table(title="Modbus Status")
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
