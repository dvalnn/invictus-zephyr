clean:
  west build -t clean

pristine:
  west build -t pristine

port := "ACM1"
minicom:
  minicom -b 115200 -D /dev/tty{{port}}

pristine := "auto"
build board proj extra_args:
  west build -p {{pristine}} -b {{board}} {{proj}} {{extra_args}}

shell := "false"
modbus_cdc := "false"
_obc_shell_flags := "-DEXTRA_DTC_OVERLAY_FILE=extra/shell.overlay -DCONF_FILE=extra/overlay_shell.conf"
_obc_modbus_cdc_flags := "-DEXTRA_DTC_OVERLAY_FILE=extra/cdc-acm.overlay -DEXTRA_CONF_FILE=extra/overlay-cdc-acm.conf"
_obc_flags := if shell != "false" { _obc_shell_flags } else { if modbus_cdc != "false" { _obc_modbus_cdc_flags } else { "" } }
obc: (build "inv2_obc" "invictus2/obc" _obc_flags )

gs: (build "inv2_obc" "invictus2/ground_station" "")


cdc_flag := if modbus_cdc != "false" { "-DEXTRA_DTC_OVERLAY_FILE=extra/cdc-acm.overlay -DEXTRA_CONF_FILE=extra/overlay-cdc-acm.conf" } else { "" }

hydra variant:
    west build -p {{pristine}} -b inv2_hydra invictus2/hydra \
      -DEXTRA_DTC_OVERLAY_FILE=extra/{{variant}}.overlay \
      -DEXTRA_CONF_FILE=extra/overlay_{{variant}}.conf \
      {{cdc_flag}}

runner := "openocd"
flash:
  west flash -r {{runner}}
