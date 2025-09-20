pristine:="auto"
runner:="openocd"

obc:
  west build -p {{pristine}} -b inv2_obc invictus2/obc

gs:
  west build -p {{pristine}} -b inv2_obc invictus2/ground_station

hydra VARIANT:
  west build -p {{pristine}} -b inv2_hydra invictus2/hydra \
    -DEXTRA_DTC_OVERLAY_FILE=extra/{{VARIANT}}.overlay \
    -DEXTRA_CONF_FILE=extra/overlay_{{VARIANT}}.conf

flash:
  west flash -r {{runner}}
