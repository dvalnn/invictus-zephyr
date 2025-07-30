#!/usr/bin/env python3
import threading
import time
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


class SimpleUARTListener:
    def __init__(self, port: str, baudrate: int = 115200):
        self.port = port
        self.baudrate = baudrate
        self.serial_connection: Optional[serial.Serial] = None
        self.running = threading.Event()
        self.thread: Optional[threading.Thread] = None
        self.messages: list[str] = []
        self.max_messages = 50

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
                time.sleep(0.05)
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
    def __init__(self, uart_port: str = "/dev/ttyACM0", baudrate: int = 115200):
        self.console = Console()
        self.uart_listener = SimpleUARTListener(uart_port, baudrate)
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

    def update_display(self):
        # Command log (left)
        command_text = Text()
        for line in self.command_log[-20:]:
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
        uart_text = Text()
        for line in self.uart_listener.messages[-20:]:
            uart_text.append(line + "\n")
        self.layout["right"].update(
            Panel(uart_text, title="UART Output", border_style="red")
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
                "  uart <message> - Send message to UART",
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
