cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(antenna_app)

target_sources(app PRIVATE
    src/radio.cpp
    src/hal/radio_hal.cpp
)
