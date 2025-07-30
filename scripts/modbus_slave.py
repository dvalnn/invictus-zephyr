#!/usr/bin/env python3

import threading

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

# Modbus data store
store = ModbusSlaveContext(
    di=ModbusSequentialDataBlock(0, [0] * 100),
    co=ModbusSequentialDataBlock(0, [False] * 100),
    hr=ModbusSequentialDataBlock(0, [0] * 100),
    ir=ModbusSequentialDataBlock(0, [0] * 100),
)
context = ModbusServerContext(slaves=store, single=True)


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


command_handlers = {
    "hr": handle_hr,
    "coil": handle_coil,
}

# Start Modbus server in a separate thread
server_thread = threading.Thread(target=run_server)
server_thread.daemon = True
server_thread.start()

print("Modbus RTU slave running...")
print("Commands:")
print("  hr,<index>,<value>        - Set holding register")
print("  coil,<index>,<True/False> - Set coil value")
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
