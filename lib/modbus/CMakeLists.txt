zephyr_library_named(invictus2_libcommon_modbus)

file(GLOB lib_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

zephyr_library_sources(${lib_sources})
