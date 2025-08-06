# Radio Communications Protocol

## RX/TX Time Windows
The protocol for syncing communication between OBC and Ground Station is as follows:
- OBC as slave
- Ground Station as master
- Ground Station is waiting for commands 
- OBC sends command
- Ground Station replies with respective ACK (with data or not)

Regarding timings, we still need to check the transmission duration of one packet in order to wait properly between transmissions.
