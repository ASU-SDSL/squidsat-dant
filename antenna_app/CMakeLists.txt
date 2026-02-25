cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(antenna_app)

file(GLOB RADIO_LIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/RadioLib/src/*.cpp
)

zephyr_library_named(radiolib)
zephyr_library_sources(${RADIO_LIB_SOURCES})
zephyr_library_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/RadioLib/src
)

target_sources(app PRIVATE
    src/radio.cpp
    src/hal/radio_hal.cpp
    src/main.cpp
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/RadioLib/src
)

target_link_libraries(app PRIVATE radiolib)