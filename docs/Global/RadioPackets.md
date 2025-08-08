# LoRa Payloads for Rocket <-> Ground Station Communications

## Packet Definition
The current packet structure is as follows:

### Packet Header
- PACKET_VERSION
- SENDER_ID
- TARGET_ID
- COMMAND_ID

### Payload
- PAYLOAD HEADER (optional)
- PAYLOAD

## Normal Mode Commands
Commands used to operate the rocket under normal circumstances

- STATUS_REQ
- STATUS_REP
- ABORT
- READY
- ARM
- FIRE
- LAUNCH_OVERRIDE
- FILL_EXEC
- FILL_STOP
- FILL_RESUME
- MANUAL_ENABLE
- MANUAL_EXEC
- ACK

## Command Descriptions

**STATUS_REQ**
Desc: Request full system info
Payload: None

**ABORT**
Desc: Enter abort state, discard all oxidizer (abort valve), close remaining valves
Payload: None

**READY**
Desc: Go to ready state, means system is ready for launch
Payload: None

**ARM**
Desc: Enter armed state, ready to fire ignition pyro
Payload: None

**FIRE**
Desc: Trigger ignition pyro, can be sent only if physical arm key is turned and the physical fire button is pressed.
Payload: None

**LAUNCH_OVERRIDE**
Desc: Force main/launch valve open
Payload: None

**FILL_EXEC**
Desc: Execute filling command
Payload: The ID of the filling program to execute, see [filling programs](#fill-exec-payload)
+ (Optional) The parameters of the program, explained [here](#custom-parameters)

**FILL_STOP**
Desc: Stop current filling state, go to Idle/Safe Idle <br> <!-- REVIEW -->
Payload: None

**FILL_RESUME**
Desc: Resume last active filling state (not idles/abort)
Payload: None

**MANUAL_TOGGLE**
Desc: Allow / Don't allow for manual commands to be executed
Payload: True / False

**MANUAL_EXEC**
Desc: Execute manual command
Payload: The ID + parameters of the manual command to execute, see [list of manual commands]

**STATUS_REP**
Desc: Send full system info
Payload: Full system info in the structure shown [here](#status-rep-payload)

**ACK**
Desc: Acknowledge command
Payload: The ID of the command received + errors during execution <!-- REVIEW -->

## Payloads
### Status Rep Payload
- **States**
    - Rocket State: 1 byte
    - Rocket Substate: 1 byte
- **Hydras Sensors**
    - Thermocouples: `uint16` (2 bytes each)
        - 6 rocket
        - 2 filling station
    - Pressures: `uint16` (2 bytes each)
        - 3 rocket
        - 3 filling station
    - *Total: 28 bytes*
- **Hydras Actuators + NAV** (Bitfields)
    - Valves: `bool` (1 bit each)
        - 4 rocket
        - 4 filling station
        - 2 filling station QR
    - E-matches: `bool` (1 bit each)
        - ignition
        - drogue chute
        - main chute
    - *Total: 13 bits (in 2 bytes)*
- **Load Cell**
    - Weights: 2 bytes each
        - 1 filling station
        - 3 for tests
    - *Total: 8 bytes*
- **GPS NAV** <!-- REVIEW -->
    - Latitude: `float` / 4 bytes (cast to `uint32`)
    - Longitude: `float` / 4 bytes (cast to `uint32`)
    - Satellites: 1 byte
    - Altitude: 2 bytes
    - Horizontal speed: 2 bytes
    - *Total: 13 bytes*
- **NAV Sensors** <!-- REVIEW -->
    - Bar Altitude: 2 × 2 bytes
    - IMU Accel XYZ: 2 × 6 bytes
    - IMU Gyr XYZ: 2 × 6 bytes
    - Mag XYZ: 6 bytes
    - *Total: 34 bytes*
- **Kalman NAV** <!-- REVIEW -->
    - Vel Z: 2 bytes
    - Accel Z: 2 bytes
    - Alt: 2 bytes
    - Max Alt: 2 bytes
    - 4 Quat: 4 × 2 bytes
    - *Total: 16 bytes*

- Max Total Payload: 101 bytes
- Packet Header: 4 bytes
- Max Packet Size: 105 bytes
- NOTE: these numbers are based on INVICTUS1.0 and need to be reviewed.


### Fill Exec Payload

The Fill Exec Payload specifies which filling program to execute and its parameters. The available filling programs, based on the filling state machine, are:

- **FILL_COPV**
    Desc: Fill the COPV tank with Nitrogen
    Parameters:
    - TargetCOPVP (COPV Pressure): default 200 bar

- **FILL_N_PRE**
    Desc: Pre-pressurize the main tank with Nitrogen
    Parameters:
    - TargetTankP (Main Tank Pressure): default 5 bar
    - TriggerTankP (Max Main Tank Pressure): default 7 bar

- **FILL_N2O**
    Desc: Fill the main tank with Nitrous Oxide
    Parameters:
    - TargetTankP (Main Tank Pressure): default 35 bar
    - TriggerTankP (Max Main Tank Pressure): default 38 bar
    - TargetWeight (Nitrous Oxide Weight): default 7 kg
    - TriggerTemp (Min Main Tank Temperature): default 2 ºC

- **FILL_N_POST**
    Desc: Post-pressurize the main tank with Nitrogen
    Parameters:
    - TargetTankP (Main Tank Pressure): default 50 bar
    - TriggerTankP (Max Main Tank Pressure): default 55 bar (optional)

### Manual Commands
**SD_LOG_START**
Desc: Start logging data to SD card
Payload: None

**SD_LOG_STOP**
Desc: Stop logging data to SD card
Payload: None

**SD_STATUS**
Desc: Request/Send SD card info
Payload: None (Ground Station), SD info (Rocket)

**VALVE_STATE**
Desc: Set the state (open/close) of a specific valve
Payload: Valve ID + desired state

**VALVE_MS**
Desc: Open a specific valve for a given duration in milliseconds
Payload: Valve ID + duration (ms)

**LOADCELL_TARE**
Desc: Tare the load cell sensor
Payload: Load cell ID

**TANK_TARE**
Desc: Tare the tank sensor
Payload: Tank ID

**ACK**
Desc: Acknowledge manual command
Payload: Manual command ID + errors during execution

Note: ACKs work like normal command acknowledges.

### Ack Errors
*?*
