cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(antenna_app)

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)

file(GLOB_RECURSE RADIOLIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/RadioLib/src/*.cpp
)
list(FILTER RADIOLIB_SOURCES EXCLUDE REGEX ".*/hal/.*\\.cpp$")

target_sources(app PRIVATE
    src/main.cpp
    src/radio.cpp
    src/hal/radio_hal.cpp
    src/can_handler.cpp  # Add your CAN file here too
    src/metrics.cpp
    ${RADIOLIB_SOURCES}
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/RadioLib/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hal
)

# 2. Add Nanopb sources. 
# Zephyr's Nanopb module provides this function automatically 
# if CONFIG_NANOPB=y is in your prj.conf.
zephyr_nanopb_sources(app src/dant_link.proto)
