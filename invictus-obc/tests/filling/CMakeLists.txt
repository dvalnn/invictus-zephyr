# tests/CMakeLists.txt

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(invictus_tests)

# Add the test source file(s)
target_sources(app PRIVATE test_filling_sm.c)

# If your tests need to include files from the main source directory:
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
target_compile_definitions(app PRIVATE UNIT_TEST)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../src/filling_sm.c)
