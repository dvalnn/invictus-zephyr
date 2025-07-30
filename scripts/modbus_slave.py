#!/usr/bin/env python3

import threading

# import serial

from pymodbus.server import StartSerialServer
from pymodbus.datastore import (
    ModbusSlaveContext,
    ModbusServerContext,
    ModbusSequentialDataBlock,
)
from pymodbus.device import ModbusDeviceIdentification

# Serial configuration
UART_MODBUS_PORT = "/dev/ttyACM1"  # Change to your device
UART_MODBUS_BAUDRATE = 115200

# Command definitions
CMD_GLOBAL_START = 0x00000001
CMD_IDLE_START = 0x00000100
CMD_OTHER_START = 0x00010000

cmd_map = {
    "CMD_STOP": CMD_GLOBAL_START,
    "CMD_ABORT": CMD_GLOBAL_START + 1,
    "CMD_PAUSE": CMD_GLOBAL_START + 2,
    "CMD_FILL_COPV": CMD_IDLE_START,
    "CMD_PRE_PRESSURIZE": CMD_IDLE_START + 1,
    "CMD_FILL_N20": CMD_IDLE_START + 2,
    "CMD_POST_PRESSURIZE": CMD_IDLE_START + 3,
    "CMD_READY": CMD_OTHER_START,
    "CMD_RESUME": CMD_OTHER_START + 1,
}

# Modbus data store
store = ModbusSlaveContext(
    di=ModbusSequentialDataBlock(0, [0] * 100),
    co=ModbusSequentialDataBlock(0, [False] * 100),
    hr=ModbusSequentialDataBlock(0, [0] * 100),
    ir=ModbusSequentialDataBlock(0, [0] * 100),
)
context = ModbusServerContext(slaves=store, single=True)

# Try to open UART for command sending


def run_server():
    identity = ModbusDeviceIdentification()
    identity.VendorName = "Pymodbus"
    identity.ProductCode = "PM"
    identity.VendorUrl = "http://github.com/riptideio/pymodbus/"
    identity.ProductName = "Modbus RTU Slave Simulator"
    identity.ModelName = "Modbus RTU Slave"
    identity.MajorMinorRevision = "1.0"

    StartSerialServer(
        context=context,
        identity=identity,
        port=UART_MODBUS_PORT,
        baudrate=UART_MODBUS_BAUDRATE,
        stopbits=1,
        bytesize=8,
        parity="N",
    )


def handle_hr(index, value):
    context[0].setValues(3, index, [value])
    print(f"Holding register {index} set to {value}")


def handle_coil(index, value):
    context[0].setValues(1, index, [bool(value)])
    print(f"Coil {index} set to {bool(value)}")


def handle_cmd(_, cmd_name):
    cmd_key = cmd_name.strip().upper()
    if cmd_key not in cmd_map:
        print(f"Unknown command: {cmd_key}")
        return
    cmd_value = cmd_map[cmd_key]
    payload = f"normal 0x{cmd_value:08x}\n"
    print(f"UART not available. Would have sent: {payload.strip()}")

    # if uart and uart.is_open:
    #     uart.write(payload.encode("utf-8"))
    #     print(f"Sent: {payload.strip()}")
    # else:
    #     print(f"UART not available. Would have sent: {payload.strip()}")


command_handlers = {
    "hr": handle_hr,
    "coil": handle_coil,
    "cmd": handle_cmd,
}

# Start Modbus server in a separate thread
server_thread = threading.Thread(target=run_server)
server_thread.daemon = True
server_thread.start()

print("Modbus RTU slave running...")
print("Commands:")
print("  hr,<index>,<value>        - Set holding register")
print("  coil,<index>,<True/False> - Set coil value")
print("  cmd,<ignored>,<CMD_NAME>  - Send high-level command to DUT")
print("Type 'exit' to quit.")

# Main input loop
while True:
    try:
        line = input("Command: ").strip()
        if line.lower() == "exit":
            print("Exiting...")
            break
        if not line:
            continue

        parts = [p.strip() for p in line.split(",")]
        if len(parts) < 2:
            print("Invalid command format. Expected at least 2 parts.")
            continue

        cmd_type = parts[0].lower()
        handler = command_handlers.get(cmd_type)
        if not handler:
            print(f"Unknown command type: {cmd_type}")
            continue

        # For "cmd", ignore the index field
        if cmd_type == "cmd":
            handler(None, parts[2] if len(parts) > 2 else parts[1])
        else:
            if len(parts) < 3:
                print("Invalid command format. Expected: type,index,value")
                continue
            index = int(parts[1])
            value = (
                int(parts[2])
                if cmd_type == "hr"
                else parts[2].lower() in ("1", "true", "yes")
            )
            handler(index, value)

    except Exception as e:
        print(f"Error: {e}")
