# Invictus2.0 Onboard Computer (OBC) project

## Compiling Project

```bash
west build -p auto -b <board> invictus2/obc
```

## Available Compilation Options

Default compilation for the custom OBC board.

```bash
west build -p auto -b inv2_obc invictus2/obc
```

Using USB CDC ACM to redirect modbus to USB device instead of RS485 bus.

```bash
west build -p auto -b inv2_obc invictus2/obc -DDTC_OVERLAY_FILE=extra/cdc-acm.overlay -DEXTRA_CONF_FILE=extra/overlay-cdc-acm.conf
```

### Other available compilation targets:

- Raspberry Pi Pico <rpi_pico>

### Adding a compilation target

- Create a DeviceTree `.overlay` file under the `boards/` directory with the name of the desired target.
For example:
    - board name: rpi_pico
    - overlay file: rpi_pico.overlay
- (If necessary) create a KConfig `.conf` fragment file under the `boards/` directory with the name of the desired target.
For example:
    - board name: rpi_pico
    - KConfig fragment file: rpi_pico.conf

When building, west will automatically detect and merge available overlays and config fragments.
If west is not automatically picking up the necessary files, check the zephyr documentation for the correct board name.
Board names and tags can be found under https://docs.zephyrproject.org/latest/boards/index.html#

## Run tests

To run unit or integration tests do the following for within the invictus' project root

    ../zephyr/scripts/twister -W -T invictus2/obc/tests/<TEST_NAME>

e.g.


    ../zephyr/scripts/twister -W -T invictus2/obc/tests/lora
