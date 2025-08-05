# Radio Communications Protocol

## Manual Command Set
    FLASH_LOG_START,
    FLASH_LOG_STOP,
    FLASH_IDS,
    FLASH_DUMP,

    VALVE_STATE,
    VALVE_MS,

    LOADCELL_TARE,
    TANK_TARE,

    manual_cmd_size,

    FLASH_LOG_START_ACK,
    FLASH_LOG_STOP_ACK,
    FLASH_IDS_ACK,
    FLASH_DUMP_ACK,
    VALVE_STATE_ACK,
    VALVE_MS_ACK,
    LOADCELL_TARE_ACK,
    TANK_TARE_ACK,

## RX/TX Time Windows
The protocol for syncing communication between OBC and Ground Station is as follows:
- OBC as slave
- Ground Station as master
- Ground Station is waiting for commands 
- OBC sends command
- Ground Station replies with respective ACK (with data or not)

Regarding timings, we still need to check the transmission duration of one packet in order to wait properly between transmissions.
