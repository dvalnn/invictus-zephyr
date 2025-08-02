#!/usr/bin/env python3

import threading
import logging
from datetime import datetime
from typing import Optional, Dict, Any

from pymodbus.server import StartSerialServer
from pymodbus.datastore import (
    ModbusSlaveContext,
    ModbusServerContext,
    ModbusSequentialDataBlock,
)
from pymodbus.device import ModbusDeviceIdentification


class ModbusSlaveSimulator:
    def __init__(self, port: str, baudrate: int):
        self.port = port
        self.baudrate = baudrate
        self.running = threading.Event()
        self.thread: Optional[threading.Thread] = None
        self.messages: list[str] = []
        self.connection_status = "Disconnected"

        # Initialize UF Hydra (Slave ID 1)
        self.uf_hydra_store = ModbusSlaveContext(
            di=None,
            co=ModbusSequentialDataBlock(0, [False] * 3),
            hr=None,
            ir=ModbusSequentialDataBlock(0, [0] * 2),
        )

        # Initialize LF Hydra (Slave ID 2)
        self.lf_hydra_store = ModbusSlaveContext(
            di=None,
            co=ModbusSequentialDataBlock(0, [False] * 3),
            hr=None,
            ir=ModbusSequentialDataBlock(0, [0] * 4),
        )

        # Create context with multiple slaves
        self.context = ModbusServerContext(
            slaves={
                1: self.uf_hydra_store,  # UF Hydra on address 1
                2: self.lf_hydra_store,  # LF Hydra on address 2
            },
            single=False,
        )

    def start(self) -> bool:
        """Start the Modbus server."""
        try:
            self.running.set()
            self.thread = threading.Thread(target=self._run_server, daemon=True)
            self.thread.start()
            self.connection_status = "Connected"
            self.add_message(
                f"Modbus RTU slave started on {self.port} at {self.baudrate} baud"
            )
            return True
        except Exception as e:
            self.connection_status = "Error"
            self.add_message(f"Error starting Modbus server: {e}")
            logging.error(f"Modbus start error: {e}")
            return False

    def stop(self):
        """Stop the Modbus server."""
        self.running.clear()
        self.connection_status = "Disconnected"
        if self.thread and self.thread.is_alive():
            self.thread.join(timeout=2)

    def add_message(self, message: str):
        """Add a timestamped message to the message queue."""
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        self.messages.append(f"[{timestamp}] MODBUS: {message}")

    def handle_hr(self, slave_id: int, index: int, value: int):
        """Handle setting a holding register value for a specific slave."""
        try:
            if slave_id in self.context:
                slave_context = self.context[slave_id]
                if slave_context.store["h"] is not None:  # Check if HR exists
                    slave_context.setValues(3, index, [value])
                    self.add_message(
                        f"Slave {slave_id}: Holding register {index} set to {value}"
                    )
                else:
                    self.add_message(
                        f"Slave {slave_id}: No holding registers available"
                    )
            else:
                self.add_message(f"Invalid slave ID: {slave_id}")
        except Exception as e:
            self.add_message(f"Error setting HR {index} on slave {slave_id}: {e}")

    def handle_coil(self, slave_id: int, index: int, value: bool):
        """Handle setting a coil value for a specific slave."""
        try:
            if slave_id in self.context:
                slave_context = self.context[slave_id]
                if slave_context.store["c"] is not None:  # Check if coils exist
                    slave_context.setValues(1, index, [bool(value)])
                    self.add_message(
                        f"Slave {slave_id}: Coil {index} set to {bool(value)}"
                    )
                else:
                    self.add_message(f"Slave {slave_id}: No coils available")
            else:
                self.add_message(f"Invalid slave ID: {slave_id}")
        except Exception as e:
            self.add_message(f"Error setting coil {index} on slave {slave_id}: {e}")

    def handle_input_register(self, slave_id: int, index: int, value: int):
        """Handle setting an input register value for a specific slave."""
        try:
            if slave_id in self.context:
                slave_context = self.context[slave_id]
                if slave_context.store["i"] is not None:  # Check if IR exists
                    slave_context.setValues(4, index, [value])
                    self.add_message(
                        f"Slave {slave_id}: Input register {index} set to {value}"
                    )
                else:
                    self.add_message(f"Slave {slave_id}: No input registers available")
            else:
                self.add_message(f"Invalid slave ID: {slave_id}")
        except Exception as e:
            self.add_message(f"Error setting IR {index} on slave {slave_id}: {e}")

    def get_register_values(
        self, slave_id: int, start: int = 0, count: int = 10
    ) -> Dict[str, Any]:
        """Get current register values for display from a specific slave."""
        try:
            if slave_id in self.context:
                slave_context = self.context[slave_id]
                values = {}

                # Try to get holding registers
                try:
                    if slave_context.store["h"] is not None:
                        hr_values = slave_context.getValues(3, start, min(count, 10))
                        for i, val in enumerate(hr_values):
                            values[f"HR{start + i}"] = val
                except Exception as e:
                    self.add_message(f"Error reading HR: {e}")

                # Try to get input registers
                try:
                    if slave_context.store["i"] is not None:
                        ir_values = slave_context.getValues(4, start, min(count, 10))
                        for i, val in enumerate(ir_values):
                            values[f"IR{start + i}"] = val
                except Exception as e:
                    self.add_message(f"Error reading IR: {e}")

                # Try to get coils
                try:
                    if slave_context.store["c"] is not None:
                        coil_values = slave_context.getValues(1, start, min(count, 10))
                        for i, val in enumerate(coil_values):
                            values[f"C{start + i}"] = int(val)
                except Exception as e:
                    self.add_message(f"Error reading coils: {e}")

                return values
            return {}
        except Exception:
            return {}

    def get_slave_info(self) -> Dict[int, Dict[str, Any]]:
        """Get information about all configured slaves."""
        info = {}
        for slave_id in [1, 2]:  # UF and LF Hydra
            if slave_id in self.context:
                slave_context = self.context[slave_id]
                slave_name = "UF Hydra" if slave_id == 1 else "LF Hydra"

                info[slave_id] = {
                    "name": slave_name,
                    "coils": slave_context.store["c"] is not None,
                    "discrete_inputs": slave_context.store["d"] is not None,
                    "holding_registers": slave_context.store["h"] is not None,
                    "input_registers": slave_context.store["i"] is not None,
                }

                # Get counts if available
                if slave_context.store["c"] is not None:
                    info[slave_id]["coil_count"] = len(slave_context.store["c"].values)
                if slave_context.store["i"] is not None:
                    info[slave_id]["ir_count"] = len(slave_context.store["i"].values)

        return info

    def _run_server(self):
        """Run the Modbus server."""
        try:
            identity = ModbusDeviceIdentification()
            identity.VendorName = "Pymodbus"
            identity.ProductCode = "PM"
            identity.VendorUrl = "http://github.com/riptideio/pymodbus/"
            identity.ProductName = "Modbus RTU Slave Simulator"
            identity.ModelName = "Modbus RTU Slave"
            identity.MajorMinorRevision = "1.0"

            StartSerialServer(
                context=self.context,
                identity=identity,
                port=self.port,
                baudrate=self.baudrate,
                stopbits=1,
                bytesize=8,
                parity="N",
            )
        except Exception as e:
            self.connection_status = "Error"
            self.add_message(f"Server error: {e}")
            logging.error(f"Modbus server error: {e}")
