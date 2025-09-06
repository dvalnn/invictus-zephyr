# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_lora)

file(GLOB app_sources src/main.c src/lora_fixture.h)

set(OBC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../invictus2/obc")
set(SERVICES_PATH "${OBC_PATH}/src/services/")

target_sources(app PRIVATE ${app_sources} ${SERVICES_PATH}/lora.c)

target_include_directories(app PRIVATE ${OBC_PATH}/include)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
