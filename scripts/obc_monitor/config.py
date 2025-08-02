#!/usr/bin/env python3

import logging
import json

from typing import Optional
from dataclasses import dataclass


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
