cmake_minimum_required(VERSION 3.20.0)

set(BOARD nucleo_f103rb CACHE STRING "")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(antenna_app)

file(GLOB_RECURSE RADIO_LIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/RadioLib/src/*.cpp
)

message(STATUS "RadioLib sources: ${RADIO_LIB_SOURCES}")

zephyr_library_named(radiolib)
zephyr_library_sources(${RADIO_LIB_SOURCES})
zephyr_library_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/RadioLib/src
)

target_sources(app PRIVATE
    src/main.cpp
    src/radio.cpp
    src/hal/radio_hal.cpp
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hal
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/RadioLib/src
)

target_link_libraries(app PRIVATE radiolib)