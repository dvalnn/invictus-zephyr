zephyr_library()

zephyr_get(SX128X_FAKE SYSBUILD GLOBAL)

file(GLOB driver_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/sx128x.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sx128x_hal.c
)

if (NOT DEFINED SX128X_FAKE)
    zephyr_library_sources(
      ${driver_sources}
      ${CMAKE_CURRENT_SOURCE_DIR}/sx128x_device.c
    )
else()
  message (STATUS "Building fake sx128x driver")
    zephyr_library_sources(
      ${driver_sources}
      ${CMAKE_CURRENT_SOURCE_DIR}/sx128x_fake_device.c
    )
endif()
