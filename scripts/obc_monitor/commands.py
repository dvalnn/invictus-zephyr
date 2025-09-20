#!/usr/bin/env python3
from __future__ import annotations

import struct
from enum import IntEnum
from typing import Dict, Any, List

from .uart import UARTListener
from .modbus import ModbusSlaveSimulator


# =============================================================================
# Constants mirrored from C header
# =============================================================================

SUPPORTED_PACKET_VERSION = 1

RADIO_CMD_SIZE = 128
HEADER_BYTES = 4  # struct cmd_header_s: 4 x uint8_t
PAYLOAD_BYTES = RADIO_CMD_SIZE - HEADER_BYTES  # 124


# =============================================================================
# Enums mirroring C enums
# =============================================================================


class RadioCommand(IntEnum):
    _RCMD_NONE = 0
    RCMD_STATUS_REQ = 1
    RCMD_ABORT = 2
    RCMD_READY = 3
    RCMD_ARM = 4
    RCMD_FIRE = 5
    RCMD_LAUNCH_OVERRIDE = 6
    RCMD_FILL_STOP = 7
    RCMD_FILL_RESUME = 8
    RCMD_MANUAL_TOGGLE = 9
    RCMD_FILL_EXEC = 10
    RCMD_MANUAL_EXEC = 11
    RCMD_STATUS_REP = 12  # (OBC -> Ground) not used here
    RCMD_ACK = 13  # (OBC -> Ground) not used here
    _RCMD_MAX = 14


class FillProgram(IntEnum):
    _FILL_PROGRAM_NONE = 0
    FILL_PROGRAM_N2 = 1
    FILL_PROGRAM_PRE_PRESS = 2
    FILL_PROGRAM_N2O = 3
    FILL_PROGRAM_POST_PRESS = 4
    _FILL_PROGRAM_MAX = 5


class ManualCmd(IntEnum):
    _MANUAL_CMD_NONE = 0
    MANUAL_CMD_SD_LOG_START = 1
    MANUAL_CMD_SD_LOG_STOP = 2
    MANUAL_CMD_SD_STATUS = 3
    MANUAL_CMD_VALVE_STATE = 4
    MANUAL_CMD_VALVE_MS = 5
    MANUAL_CMD_LOADCELL_TARE = 6
    MANUAL_CMD_TANK_TARE = 7
    _MANUAL_CMD_MAX = 8


# =============================================================================
# Defaults (user-friendly units)
#   - bar  -> decibar (×10)
#   - kg   -> grams   (×1000)
#   - °C   -> deci°C  (×10)
# =============================================================================

FILL_DEFAULTS: Dict[FillProgram, Dict[str, Any]] = {
    FillProgram.FILL_PROGRAM_N2: {
        "TargetN2Bar": 200.0,  # 200 bar -> decibar
    },
    FillProgram.FILL_PROGRAM_PRE_PRESS: {
        "TargetTankBar": 5.0,
        "TriggerTankBar": 7.0,  # optional; if you want to omit, pass TriggerTankBar=None
    },
    FillProgram.FILL_PROGRAM_N2O: {
        # Header struct encodes only weight + trigger temp (no pressures)
        "TargetWeightKg": 7.0,  # 7 kg -> grams
        "TriggerTempC": 2.0,  # 2 °C -> deci°C
    },
    FillProgram.FILL_PROGRAM_POST_PRESS: {
        "TargetTankBar": 50.0,
        "TriggerTankBar": 55.0,  # optional; set None to encode 0xFFFF
    },
}


# Accept legacy names from your previous CLI and map to the new program IDs
PROGRAM_NAME_ALIASES: Dict[str, FillProgram] = {
    "FILL_COPV": FillProgram.FILL_PROGRAM_N2,
    "FILL_N_PRE": FillProgram.FILL_PROGRAM_PRE_PRESS,
    "FILL_N2O": FillProgram.FILL_PROGRAM_N2O,
    "FILL_N_POST": FillProgram.FILL_PROGRAM_POST_PRESS,
    # Native names too:
    "N2": FillProgram.FILL_PROGRAM_N2,
    "PRE_PRESS": FillProgram.FILL_PROGRAM_PRE_PRESS,
    "POST_PRESS": FillProgram.FILL_PROGRAM_POST_PRESS,
}


# =============================================================================
# Packet Builder
# =============================================================================


class PacketBuilder:
    """
    Builds 128-byte packets to match the C radio command structs.
    Endianness:
      - 'little' (default): '<'  (Zephyr/ARM RP2040)
      - 'big': '>'
    """

    def __init__(self, sender_id: int, target_id: int, endianness: str = "little"):
        self.sender_id = sender_id & 0xFF
        self.target_id = target_id & 0xFF
        self.set_endianness(endianness)

    def set_endianness(self, endianness: str) -> None:
        if endianness not in ("little", "big"):
            raise ValueError("endianness must be 'little' or 'big'")
        self._end_sym = "<" if endianness == "little" else ">"
        self.endianness = endianness

    def _pack_header(self, cmd_id: RadioCommand) -> bytes:
        # struct cmd_header_s: 4 x uint8_t
        return struct.pack(
            f"{self._end_sym}BBBB",
            SUPPORTED_PACKET_VERSION & 0xFF,
            self.sender_id,
            self.target_id,
            int(cmd_id) & 0xFF,
        )

    def build(self, cmd_id: RadioCommand, payload: bytes = b"") -> bytes:
        if len(payload) > PAYLOAD_BYTES:
            raise ValueError(f"payload too large ({len(payload)} > {PAYLOAD_BYTES})")
        header = self._pack_header(cmd_id)
        pad_len = PAYLOAD_BYTES - len(payload)
        return header + payload + (b"\x00" * pad_len)


# =============================================================================
# Payload encoders (mirror C structs)
# =============================================================================


def _u16(end_sym: str, val: int) -> bytes:
    return struct.pack(f"{end_sym}H", val & 0xFFFF)


def _u32(end_sym: str, val: int) -> bytes:
    return struct.pack(f"{end_sym}I", val & 0xFFFFFFFF)


def _i16(end_sym: str, val: int) -> bytes:
    # signed 16-bit
    if not (-32768 <= val <= 32767):
        raise ValueError("i16 out of range")
    return struct.pack(f"{end_sym}h", val)


def _bool8(val: bool) -> int:
    return 1 if bool(val) else 0


def encode_fill_exec(
    end_sym: str, program: FillProgram, params: Dict[str, Any]
) -> bytes:
    """
    Encodes struct fill_exec_s:
      uint8_t program_id;
      uint8_t params[PAYLOAD_BYTES - 1];

    Program-specific param structs:
      - N2:     uint16_t target_N2_deci_bar;
      - PRE/POST:
               uint16_t target_tank_deci_bar;
               uint16_t trigger_tank_deci_bar; // 0xFFFF if unused
      - N2O:    uint16_t target_weight_grams;
               int16_t  trigger_temp_deci_c;
    """
    payload = bytearray()
    payload.append(int(program) & 0xFF)

    if program == FillProgram.FILL_PROGRAM_N2:
        # Expect TargetN2Bar (bar -> decibar)
        target_db = int(round(float(params["TargetN2Bar"]) * 10.0))
        payload += _u16(end_sym, target_db)

    elif program in (
        FillProgram.FILL_PROGRAM_PRE_PRESS,
        FillProgram.FILL_PROGRAM_POST_PRESS,
    ):
        target_db = int(round(float(params["TargetTankBar"]) * 10.0))
        trig = params.get("TriggerTankBar", None)
        if trig is None:
            trigger_db = 0xFFFF  # optional sentinel
        else:
            trigger_db = int(round(float(trig) * 10.0))
        payload += _u16(end_sym, target_db)
        payload += _u16(end_sym, trigger_db)

    elif program == FillProgram.FILL_PROGRAM_N2O:
        # Expect TargetWeightKg (kg -> grams), TriggerTempC (°C -> deci°C)
        grams = int(round(float(params["TargetWeightKg"]) * 1000.0))
        dC = int(round(float(params["TriggerTempC"]) * 10.0))
        payload += _u16(end_sym, grams)
        payload += _i16(end_sym, dC)

    else:
        raise ValueError(f"Unsupported fill program: {program}")

    return bytes(payload)


def encode_manual_exec(
    end_sym: str, manual_cmd: ManualCmd, params: Dict[str, Any]
) -> bytes:
    """
    Encodes struct manual_exec_s:
      uint8_t manual_cmd_id;
      uint8_t params[PAYLOAD_BYTES - 1];

    Manual payloads:
      - VALVE_STATE: struct { uint8_t valve_id; uint8_t open; }
      - VALVE_MS:    struct { uint8_t valve_id; uint32_t duration_ms; }
      - LOADCELL_TARE / TANK_TARE: struct { uint8_t id; }
      - SD_LOG_START / SD_LOG_STOP / SD_STATUS: no extra params
    """
    payload = bytearray()
    payload.append(int(manual_cmd) & 0xFF)

    if manual_cmd == ManualCmd.MANUAL_CMD_VALVE_STATE:
        valve_id = int(params["ValveID"]) & 0xFF
        open_flag = _bool8(params.get("Open", True)) & 0xFF
        payload.append(valve_id)
        payload.append(open_flag)

    elif manual_cmd == ManualCmd.MANUAL_CMD_VALVE_MS:
        valve_id = int(params["ValveID"]) & 0xFF
        duration = int(params["DurationMs"])
        payload.append(valve_id)
        payload += _u32(end_sym, duration)

    elif manual_cmd in (
        ManualCmd.MANUAL_CMD_LOADCELL_TARE,
        ManualCmd.MANUAL_CMD_TANK_TARE,
    ):
        payload.append(int(params["ID"]) & 0xFF)

    elif manual_cmd in (
        ManualCmd.MANUAL_CMD_SD_LOG_START,
        ManualCmd.MANUAL_CMD_SD_LOG_STOP,
        ManualCmd.MANUAL_CMD_SD_STATUS,
    ):
        # no extra params
        pass

    else:
        raise ValueError(f"Unsupported manual command: {manual_cmd}")

    return bytes(payload)


# =============================================================================
# CLI helpers (parse "Key=Value" tokens)
# =============================================================================


def parse_kv(tokens: List[str]) -> Dict[str, str]:
    out: Dict[str, str] = {}
    for t in tokens:
        if "=" not in t:
            continue
        k, v = t.split("=", 1)
        out[k.strip()] = v.strip()
    return out


def boolish(s: str) -> bool:
    return s.lower() in ("1", "true", "yes", "on", "enable", "enabled")


# =============================================================================
# Command Processor (refactored)
# =============================================================================


class CommandProcessor:
    def __init__(
        self,
        uart_listener: UARTListener,
        modbus_simulator: ModbusSlaveSimulator,
        *,
        sender_id: int = 0x01,
        target_id: int = 0x10,
        endianness: str = "little",  # "little" for RP2040/Zephyr, "big" for tests
    ):
        self.uart_listener = uart_listener
        self.modbus_simulator = modbus_simulator
        self.command_history: list[str] = []
        self.max_history = 100

        self.packet = PacketBuilder(
            sender_id=sender_id, target_id=target_id, endianness=endianness
        )

        # simple commands without payload (per C header)
        self._simple_cmds: Dict[str, RadioCommand] = {
            "STATUS_REQ": RadioCommand.RCMD_STATUS_REQ,
            "ABORT": RadioCommand.RCMD_ABORT,
            "READY": RadioCommand.RCMD_READY,
            "ARM": RadioCommand.RCMD_ARM,
            "FIRE": RadioCommand.RCMD_FIRE,
            "LAUNCH_OVERRIDE": RadioCommand.RCMD_LAUNCH_OVERRIDE,
            "FILL_STOP": RadioCommand.RCMD_FILL_STOP,
            "FILL_RESUME": RadioCommand.RCMD_FILL_RESUME,
            "MANUAL_TOGGLE": RadioCommand.RCMD_MANUAL_TOGGLE,  # note: no payload in C header
        }

    # ---------- internal send helper ----------
    def _send_bytes(self, payload: bytes) -> bool:
        return self.uart_listener.send_bytes(payload)

    # ---------- public API ----------
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
                    "  uart <message> - Send message to UART (raw passthrough)",
                    "  cmd <COMMAND> [args] - Send radio command",
                    "    Simple: " + ", ".join(self._simple_cmds.keys()),
                    "    Fill Exec: FILL_EXEC <PROGRAM> [Key=Value ...]",
                    "      Programs: FILL_COPV | FILL_N_PRE | FILL_N2O | FILL_N_POST",
                    "      Examples:",
                    "        cmd FILL_EXEC FILL_N2O TargetWeightKg=8 TriggerTempC=1.5",
                    "        cmd FILL_EXEC FILL_COPV TargetN2Bar=180",
                    "        cmd FILL_EXEC FILL_N_PRE TargetTankBar=6 TriggerTankBar=7.5",
                    "    Manual Exec: MANUAL_EXEC <SUBCMD> [Key=Value ...]",
                    "      Subcmds: SD_LOG_START | SD_LOG_STOP | SD_STATUS |",
                    "               VALVE_STATE (ValveID=, Open=true/false) |",
                    "               VALVE_MS (ValveID=, DurationMs=) |",
                    "               LOADCELL_TARE (ID=) | TANK_TARE (ID=)",
                    "  modbus <slave_id> <hr|coil|ir> <index> <value> - Set Modbus (unchanged)",
                    "  status | history | clear | save-config [file] | quit/exit",
                ]
            )

        elif cmd == "uart":
            if len(parts) > 1:
                message = " ".join(parts[1:])
                if self.uart_listener.send(message):
                    responses.append(f"✓ Sent: {message}")
                else:
                    responses.append("✗ Failed to send UART passthrough")
            else:
                responses.append("Usage: uart <message>")

        elif cmd == "cmd":
            if len(parts) < 2:
                responses.append(
                    "Usage: cmd <COMMAND> [...]  (type 'help' for examples)"
                )
                return responses

            sub = parts[1].upper()

            # ---------- simple commands (no payload) ----------
            if sub in self._simple_cmds:
                pkt = self.packet.build(self._simple_cmds[sub], b"")
                ok = self._send_bytes(pkt)
                preview = pkt[:8].hex()
                responses.append(f"Built {sub} (128 bytes), head={preview}")
                responses.append("✓ Sent" if ok else "✗ Send failed")
                return responses

            # ---------- FILL_EXEC ----------
            if sub == "FILL_EXEC":
                if len(parts) < 3:
                    responses.append("Usage: cmd FILL_EXEC <PROGRAM> [Key=Value ...]")
                    return responses

                prog_name = parts[2].upper()
                prog = PROGRAM_NAME_ALIASES.get(prog_name)
                if prog is None and prog_name in [p.name for p in FillProgram]:
                    # allow raw enum name, e.g., FILL_PROGRAM_N2
                    prog = FillProgram[prog_name]
                if prog is None:
                    responses.append(f"✗ Unknown fill program: {prog_name}")
                    return responses

                kv = parse_kv(parts[3:])
                # start with defaults
                defaults = FILL_DEFAULTS[prog].copy()
                # merge overrides (convert strings to numbers where appropriate)
                merged: Dict[str, Any] = {}
                for k, v in defaults.items():
                    if k in kv:
                        try:
                            merged[k] = float(kv[k])
                        except ValueError:
                            # allow 'None' to unset optional trigger
                            merged[k] = None if kv[k].lower() == "none" else kv[k]
                    else:
                        merged[k] = v

                try:
                    payload = encode_fill_exec(self.packet._end_sym, prog, merged)
                except Exception as e:
                    responses.append(f"✗ FILL_EXEC encode error: {e}")
                    return responses

                pkt = self.packet.build(RadioCommand.RCMD_FILL_EXEC, payload)
                ok = self._send_bytes(pkt)
                preview = pkt[:8].hex()
                responses.append(
                    f"Built FILL_EXEC/{prog.name} (payload {len(payload)}B, total 128B), head={preview}"
                )
                responses.append("✓ Sent" if ok else "✗ Send failed")
                return responses

            # ---------- MANUAL_EXEC ----------
            if sub == "MANUAL_EXEC":
                if len(parts) < 3:
                    responses.append("Usage: cmd MANUAL_EXEC <SUBCMD> [Key=Value ...]")
                    return responses

                subcmd = parts[2].upper()
                try:
                    manual_cmd = ManualCmd[f"MANUAL_CMD_{subcmd}"]
                except KeyError:
                    responses.append(f"✗ Unknown manual subcommand: {subcmd}")
                    return responses

                kv = parse_kv(parts[3:])
                # Friendly coercions
                if manual_cmd == ManualCmd.MANUAL_CMD_VALVE_STATE:
                    if "ValveID" not in kv:
                        responses.append(
                            "✗ VALVE_STATE requires ValveID and (optional) Open=true/false"
                        )
                        return responses
                    if "Open" in kv:
                        kv["Open"] = boolish(kv["Open"])
                elif manual_cmd == ManualCmd.MANUAL_CMD_VALVE_MS:
                    if "ValveID" not in kv or "DurationMs" not in kv:
                        responses.append(
                            "✗ VALVE_MS requires ValveID=<id> DurationMs=<ms>"
                        )
                        return responses
                elif manual_cmd in (
                    ManualCmd.MANUAL_CMD_LOADCELL_TARE,
                    ManualCmd.MANUAL_CMD_TANK_TARE,
                ):
                    if "ID" not in kv:
                        responses.append(f"✗ {subcmd} requires ID=<num>")
                        return responses

                try:
                    payload = encode_manual_exec(self.packet._end_sym, manual_cmd, kv)
                except Exception as e:
                    responses.append(f"✗ MANUAL_EXEC encode error: {e}")
                    return responses

                pkt = self.packet.build(RadioCommand.RCMD_MANUAL_EXEC, payload)
                ok = self._send_bytes(pkt)
                preview = pkt[:8].hex()
                responses.append(
                    f"Built MANUAL_EXEC/{manual_cmd.name} (payload {len(payload)}B, total 128B), head={preview}"
                )
                responses.append("✓ Sent" if ok else "✗ Send failed")
                return responses

            responses.append(f"✗ Unknown command: {sub}")
            responses.append("Type 'help' for available commands")

        elif cmd == "modbus":
            if len(parts) < 2 or parts[1].lower() == "restart":
                self.modbus_simulator.restart()
                responses.append("✓ Modbus simulator restarted")
                return responses

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
                    f"Packet endianness: {self.packet.endianness}",
                ]
            )

        elif cmd == "history":
            responses.append("Command history:")
            for i, hist_cmd in enumerate(self.command_history[-10:], 1):
                responses.append(f"  {i:2d}. {hist_cmd}")

        elif cmd == "clear":
            return ["Command log cleared"]

        elif cmd == "save-config":
            config_file = parts[1] if len(parts) > 1 else "config.json"
            try:
                responses.append(
                    f"Config save feature needs implementation for {config_file}"
                )
            except Exception as e:
                responses.append(f"✗ Failed to save config: {e}")

        elif cmd in ("quit", "exit"):
            responses.append("Shutting down...")
            return responses + ["QUIT"]

        else:
            responses.append(f"✗ Unknown command: {command}")
            responses.append("Type 'help' for available commands")

        return responses
