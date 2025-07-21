#!/usr/bin/env python3

import threading
import time
from pymodbus.server import StartSerialServer
from pymodbus.datastore import ModbusSlaveContext, ModbusServerContext
from pymodbus.datastore import ModbusSequentialDataBlock
from pymodbus.device import ModbusDeviceIdentification

# Create a global Modbus datastore with initial values for
# discrete inputs, coils, holding registers, and input registers.
store = ModbusSlaveContext(
    di=ModbusSequentialDataBlock(0, [0]*100),
    co=ModbusSequentialDataBlock(0, [False]*100),
    hr=ModbusSequentialDataBlock(0, [0]*100),
    ir=ModbusSequentialDataBlock(0, [0]*100)
)
# Use single slave mode for simplicity.
context = ModbusServerContext(slaves=store, single=True)

def run_server():
    # Set up device identity (optional, but can be useful)
    identity = ModbusDeviceIdentification()
    identity.VendorName = 'Pymodbus'
    identity.ProductCode = 'PM'
    identity.VendorUrl = 'http://github.com/riptideio/pymodbus/'
    identity.ProductName = 'Modbus RTU Slave Simulator'
    identity.ModelName = 'Modbus RTU Slave'
    identity.MajorMinorRevision = '1.0'

    # Start the serial server.
    # Change port to your serial port (e.g., 'COM3' on Windows or '/dev/ttyUSB0' on Linux)
    StartSerialServer(context=context, identity=identity, port='/dev/ttyACM0',
                        baudrate=115200, stopbits=1, bytesize=8, parity='N')

# Run the Modbus RTU server in a separate thread so we can interactively change values.
server_thread = threading.Thread(target=run_server)
server_thread.daemon = True
server_thread.start()

print("Modbus RTU slave running...")
print("You can update registers or coils using the following commands:")
print("  For a holding register: hr,<index>,<value>     e.g., hr,10,123")
print("  For a coil:             coil,<index>,<True/False> e.g., coil,5,True")
print("Type 'exit' to quit.")

while True:
    try:
        # Read user input from the command line
        command = input("Command: ").strip()
        if command.lower() == 'exit':
            print("Exiting...")
            break
        if not command:
            continue
        parts = command.split(',')
        if len(parts) != 3:
            print("Invalid format. Please use: type,index,value")
            continue
        
        typ, index_str, value_str = parts
        index = int(index_str.strip())
        typ = typ.strip().lower()
        
        # Update holding register values (function code 3)
        if typ == "hr":
            value = int(value_str.strip())
            # In pymodbus, function code 3 is for holding registers.
            context[0].setValues(3, index, [value])
            print(f"Holding register {index} set to {value}")
        # Update coil values (function code 1)
        elif typ == "coil":
            # Accept various representations of a Boolean
            value = value_str.strip().lower() in ("1", "true", "yes")
            context[0].setValues(1, index, [value])
            print(f"Coil {index} set to {value}")
        else:
            print("Unknown type. Use 'hr' for holding registers or 'coil' for coils.")
    except Exception as e:
        print(f"Error: {e}")
