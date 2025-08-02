#!/usr/bin/env python3

from .uart import UARTListener
from .modbus import ModbusSlaveSimulator

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
