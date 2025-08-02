#!/usr/bin/env python3

import logging
import argparse
import os

from .config import Config
from .tui import OBCMonitorTUI


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
    app = OBCMonitorTUI(config)
    try:
        app.start()
    except KeyboardInterrupt:
        print("\nApplication interrupted by user")
    except Exception as e:
        print(f"Application error: {e}")
        logging.error(f"Application error: {e}", exc_info=True)


if __name__ == "__main__":
    main()
