cmake_minimum_required(VERSION 3.13)
add_library(pico_neopixel INTERFACE)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(Jog3k C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_executable(jog3k src/jog3k.cpp src/usb_descriptors.cpp lib/Adafruit_NeoPixel.cpp lib/Adafruit_NeoPixel.hpp lib/BitBang_I2C.cpp lib/BitBang_I2C.h lib/ss_oled.cpp lib/ss_oled.h)
target_include_directories(jog3k PRIVATE include/ lib/)

pico_generate_pio_header(jog3k ${CMAKE_CURRENT_LIST_DIR}/src/ws2812byte.pio)

add_compile_definitions(PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)

target_link_libraries(jog3k PRIVATE pico_stdlib tinyusb_device tinyusb_board hardware_pwm hardware_pio hardware_i2c pico_multicore hardware_flash)

pico_add_extra_outputs(jog3k)
