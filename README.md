# Invictus2.0

## Building and Running Invictus2 board firmware

Build application:
```bash
west build -p auto -b <board> invictus2/<app>
```

merging additional overlay and Kconfig fragments:
```bash
west build -p auto -b <board> invictus2/<app> -DEXTRA_DTC_OVERLAY_FILE=<dtc_fragment>.overlay -DEXTRA_CONF_FILE=<kconfig_fragment>.conf
```

Building for the raspberry pi pico (`rpi_pico`) is adequate for testing purposes.
Aditionally, definitions for the custom invictus boards exist, under the names:

```
inv2_obc
inv2_nav
inv2_hydra
inv2_psat
```

flash to board:
```bash
west flash
```

Build for local simulation:
```bash
west build -p auto -b native_sim invictus2/<app>
```
or 
```bash
west build -p auto -b native_sim/native/64 invictus2/<app>
```

run emulation/simulation:
```bash
west build -t run
```

run tests:
```bash
west twister -T test/ -G
west twister -T invictus2/obc/tests/lora/unit-testing

```

## Contributing

### Environment setup

1. Install Zephyr dependencies 
   - Follow the instructions in the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
2. Install west
   - Follow the instructions in the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
3. Initialize the Zephyr workspace from the manifest repository
   ```bash
   mkdir zephyrworkspace
   cd zephyrworkspace
   west init -m https://github.com/dvalnn/pst_zephyr_manifest
   west update
   ```
4. Export the CMake package
   ```bash
   west zephyr-export
   ```
5. Install the required Python packages
   ```bash
   west packages pip --install
   ```
6. Install the SDK
   ```bash
   west sdk install
   ```
   or follow the instructions in the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/toolchains/zephyr_sdk.html#toolchain-zephyr-sdk-install) to install the SDK manually.

### Contributing

1. Fork this repository

2. Clone your fork to the zephyr workspace
   ```bash
   cd zephyrworkspace
   git clone <your_fork_url>/invictus-zephyr
   ```

2. Create a new branch (`git checkout -b feature-branch`)

3. Make your changes
