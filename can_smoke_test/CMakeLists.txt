cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(can_smoke_test)

# This target exercises raw CAN without radio or protobuf initialization.
target_sources(app PRIVATE src/main.cpp ../antenna_app/src/can_handler.cpp)
