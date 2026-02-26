cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(antenna_app)

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)

# 1. Add your standard sources first
target_sources(app PRIVATE
    src/main.cpp
    src/radio.cpp
    src/hal/radio_hal.cpp
    src/can_handler.cpp  # Add your CAN file here too
)

# 2. Add Nanopb sources. 
# Zephyr's Nanopb module provides this function automatically 
# if CONFIG_NANOPB=y is in your prj.conf.
zephyr_nanopb_sources(app src/dant_link.proto)