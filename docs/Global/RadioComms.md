# Radio Communications Protocol

## Packet Definition
The current packet structure is as follows:
- START BYTE(0x55)
- PACKET_VERSION
- COMMAND
- TARGET_ID 
- SENDER_ID 
- PAYLOAD LENGTH 
- H_CRC1
- H_CRC2
- PAYLOAD 
- CRC1 
- CRC2

## Command Set
    //shared commands
    STATUS,
    ABORT,
    EXEC_PROG,
    STOP_PROG,
    FILLING,
    MANUAL,
    MANUAL_EXEC,
    READY,
    ARM,
    ALLOW_LAUNCH,
    RESUME_PROG,
    FIRE,
    // used to get number of commands
    size, 

    //ACKs
    STATUS_ACK,
    LOG_ACK,
    ABORT_ACK,
    EXEC_PROG_ACK,
    STOP_PROG_ACK,
    FILLING_ACK,
    MANUAL_ACK,
    MANUAL_EXEC_ACK,
    READY_ACK,
    ARM_ACK,
    ALLOW_LAUNCH_ACK,
    RESUME_PROG_ACK,
    FIRE_ACK,

## Manual Command Set
    FLASH_LOG_START,
    FLASH_LOG_STOP,
    FLASH_IDS,
    FLASH_DUMP,

    VALVE_STATE,
    VALVE_MS,

    IMU_CALIBRATE,
    BAROMETER_CALIBRATE,
    KALMAN_CALIBRATE,
    LOADCELL_CALIBRATE,

    LOADCELL_TARE,
    TANK_TARE,

    manual_cmd_size,

    FLASH_LOG_START_ACK,
    FLASH_LOG_STOP_ACK,
    FLASH_IDS_ACK,
    FLASH_DUMP_ACK,
    VALVE_STATE_ACK,
    VALVE_MS_ACK,
    IMU_CALIBRATE_ACK,
    BAROMETER_CALIBRATE_ACK,
    KALMAN_CALIBRATE_ACK,
    LOADCELL_CALIBRATE_ACK,
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