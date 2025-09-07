#!/usr/bin/env python3

import threading
import logging
import serial
from datetime import datetime
from typing import Optional


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

    def send_bytes(self, packet: bytes) -> bool:
        """Send raw bytes via UART."""
        try:
            if self.serial_connection and self.serial_connection.is_open:
                written = self.serial_connection.write(packet)
                self.serial_connection.flush()
                self.bytes_sent += written
                self.add_message(
                    f"TX: {written} bytes ({packet[:16].hex()}...)"
                    if len(packet) > 16
                    else f"TX: {packet.hex()}"
                )
                return written == len(packet)

        except Exception as e:
            self.add_message(f"Send Error: {e}")
            logging.error(f"UART send error: {e}")

        return False
