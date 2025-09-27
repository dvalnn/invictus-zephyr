 
from pymodbus.client import ModbusSerialClient
import time

def main():
    client = ModbusSerialClient(
        port="/dev/ttyACM0",  # Adjust to your serial port
        baudrate=115200,
        timeout=1,
        parity="N",
        stopbits=1,
        bytesize=8
    )

    if not client.connect():
        print("Failed to connect to Modbus RTU server.")
        return

    print("Connected. Listening for Modbus responses...")

    try:
        while True:
            response = client.read_holding_registers(
                address=0,      #starting register
                count=8,       #number of registers
                slave=1         #slaveID
            )

            if response.isError():
                print("Error reading:", response)
            else:
                print("Received registers:", response.registers)

            time.sleep(1)  # poll every second

    except KeyboardInterrupt:
        print("\nStopping client...")

    finally:
        client.close()

if __name__ == "__main__":
    main()