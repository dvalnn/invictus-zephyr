import threading
import serial

UART_CMD_SHELL_PORT = "/dev/ttyACM0"  # Change to your device
UART_CMD_SHELL_BAUDRATE = 115200

# Command definitions
CMD_GLOBAL_START = 0x00000001
CMD_IDLE_START = 0x00000100
CMD_OTHER_START = 0x00010000

cmd_map = {
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

try:
    uart = serial.Serial(UART_CMD_SHELL_PORT, UART_CMD_SHELL_BAUDRATE, timeout=1)
except Exception as _:
    uart = None


def handle_cmd(_, cmd_name):
    cmd_key = cmd_name.strip().upper()
    if cmd_key not in cmd_map:
        print(f"Unknown command: {cmd_key}")
        return
    cmd_value = cmd_map[cmd_key]
    payload = f"normal 0x{cmd_value:08x}\n"
    if uart and uart.is_open:
        uart.write(payload.encode("utf-8"))
        print(f"S: `{payload.strip()}`")
    else:
        print(f"[WARN] UART not available. Would have sent: {payload.strip()}")


def uart_listen_thread():
    if uart is None:
        print("[ERROR] UART not initialized.")
        return

    while True:
        try:
            if uart.in_waiting > 0:
                line = uart.readline().decode("utf-8").strip()
                if line:
                    print(f"R: {line}")
                    handle_cmd(None, line)
        except serial.SerialException as e:
            print(f"Serial error: {e}")
            break
        except Exception as e:
            print(f"Error: {e}")
            break


# Start UART listening thread
uart_thread = threading.Thread(target=uart_listen_thread)
uart_thread.daemon = True
uart_thread.start()

print("running...")
print("Commands:")
print("  cmd,<CMD_NAME>  - Send high-level command to DUT")
print("Type 'exit' to quit.")
print("Type 'help' for command list.")

while True:
    try:
        line = input().strip()
        if not line:
            continue

        if line.lower() == "exit":
            print("Exiting...")
            break

        if line.lower() == "help":
            print("[HELP] Available commands:")
            for cmd in cmd_map.keys():
                print(f"[HELP]     cmd,{cmd} - Send command {cmd}")
            continue

        parts = [p.strip() for p in line.split(",")]
        if len(parts) < 2:
            print("Invalid command format. Expected at least 2 parts.")
            continue

        cmd_type = parts[0].lower()
        if cmd_type != "cmd":
            print(f"[WARN] Unknown command type: {cmd_type}")
            print("Type 'help' for command list.")
            continue

        if len(parts) < 3:
            print("[ERROR] Invalid command format. Expected: cmd,<CMD_NAME>")
            continue

        handle_cmd(parts[0], parts[1])

    except Exception as e:
        print(f"Error: {e}")
