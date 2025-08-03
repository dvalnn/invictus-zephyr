zephyr_library()

file(GLOB driver_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
zephyr_library_sources(${driver_sources})
