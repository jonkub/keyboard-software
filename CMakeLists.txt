cmake_minimum_required(VERSION 3.20.0)

list(APPEND BOARD_ROOT ${CMAKE_SOURCE_DIR})
list(APPEND DTS_ROOT ${CMAKE_SOURCE_DIR}/drivers/zephyr)
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/drivers)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(vpk)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fno-threadsafe-statics")
# FILE(GLOB app_sources src/*.cpp)
# target_sources(app PRIVATE ${app_sources})
target_sources(app PRIVATE src/main.cpp src/keyboard.cpp src/usb_hid.cpp src/led.cpp src/bat.cpp src/bluetooth/bluetooth.cpp src/bluetooth/hog.cpp)
