# LoRa Payloads for Rocket <-> Ground Station Communications

## Packet Definition
The current packet structure is as follows:

### Packet Header
- PACKET\_VERSION
- SENDER\_ID
- TARGET\_ID
- COMMAND\_ID

### Payload
- PAYLOAD HEADER (optional)
- PAYLOAD

### Normal Mode Commands
Commands used to operate the rocket under normal circumstances

- STATUS\_REQ 
- STATUS\_REP 
- ABORT 
- READY 
- ARM 
- FIRE 
- LAUNCH_OVERRIDE 
- FILL\_EXEC 
- FILL\_STOP 
- FILL\_RESUME 
- MANUAL\_ENABLE 
- MANUAL\_EXEC 
- ACK 

### Command Descriptions

#### STATUS\_REQ 
desc: request full system info
payload: NULL

#### STATUS\_REP 
desc: full system info
payload: // REVIEW @alex

#### ABORT 
desc: EJECT ALL FUEL, close all other valves
payload: NULL

#### READY 
desc: go to ready state
payload: NULL

#### ARM 
desc: go to armed state
payload: NULL

#### FIRE 
desc: trigger e-matches
payload: NULL

#### LAUNCH_OVERRIDE 
desc: force main/launch valve open
payload: NULL

#### FILL\_EXEC 
desc: execute filling command
payload: // REVIEW @alex

#### FILL\_STOP 
desc: stop ongoing filling state
payload: NULL

#### FILL\_RESUME 
desc: resume last filling state
payload: NULL

#### MANUAL\_ENABLE 
desc: allow for manual commands to be executed
payload: True / False or NULL and act as a Toggle (?)

#### MANUAL\_EXEC 
desc: execute manual command
payload: // REVIEW @alex

#### ACK 
desc: acknowledge command
payload: // REVIEW @alex
