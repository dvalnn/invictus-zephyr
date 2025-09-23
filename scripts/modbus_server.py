from pymodbus.server import StartSerialServer
from pymodbus.datastore import ModbusSlaveContext, ModbusServerContext
from pymodbus.datastore.store import ModbusSequentialDataBlock
import logging

logging.basicConfig(level=logging.INFO)

# Serial port settings (adjust for your system)
serial_port = "/dev/ttyUSB0"  # Change as needed
baud_rate = 19200
parity = "N"   # Options: "N" (None), "E" (Even), "O" (Odd)
stopbits = 1
bytesize = 8

# Create slaves (IDs 1 to 100, each with 180 holding registers initialized to 0)
slaves = {
    slave_id: ModbusSlaveContext(hr=ModbusSequentialDataBlock(0, [0] * 180))
    for slave_id in range(1, 101)
}

context = ModbusServerContext(slaves=slaves, single=False)

try:
    logging.info(f"Starting Modbus RTU server on {serial_port} @ {baud_rate} baud")
    StartSerialServer(
        context,
        port=serial_port,
        baudrate=baud_rate,
        parity=parity,
        stopbits=stopbits,
        bytesize=bytesize,
        framer=None,  # Default RTU framer will be used
    )

except Exception as e:
    logging.error(f"Error starting RTU server: {e}")