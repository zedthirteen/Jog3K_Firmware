#SET(GCC_COVERAGE_COMPILE_FLAGS "-L")
#add_library(pico_neopixel INTERFACE)

cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(app_main C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(FAMILY rp2040)
set(BOARD pico_sdk)
set(DEBUG 1)

add_compile_definitions(PICO_DEFAULT_UART=0 PICO_DEFAULT_UART_TX_PIN=12 PICO_DEFAULT_UART_RX_PIN=13 JOG_MODULE=0)

# set(TINYUSB_FAMILY_PROJECT_NAME_PREFIX "tinyusb_host_")
add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})
pico_sdk_init()
add_subdirectory(i2c_slave)
add_executable(app_main
    ${CMAKE_CURRENT_LIST_DIR}/ds4_app.c
    ${CMAKE_CURRENT_LIST_DIR}/jog2k_app.c
    ${CMAKE_CURRENT_LIST_DIR}/hid_app.c
    ${CMAKE_CURRENT_LIST_DIR}/msc_app.c
    ${CMAKE_CURRENT_LIST_DIR}/i2c_jogger.c
    ${CMAKE_CURRENT_LIST_DIR}/app_main.c
)

target_include_directories(app_main PUBLIC ${CMAKE_CURRENT_LIST_DIR})

#${PICO_SDK_PATH}/lib/tinyusb/hw/bsp/${FAMILY}/family.c)
# Pull in pico libraries that we need
# target_link_libraries(pico_neopixel INTERFACE pico_stdlib hardware_pio pico_malloc pico_mem_ops)
target_link_libraries(app_main PUBLIC i2c_slave pico_stdlib pico_multicore tinyusb_host tinyusb_board hardware_flash hardware_i2c )
# add_subdirectory(${PICO_TINYUSB_PATH}/examples/host tinyusb_host_examples)
#add_subdirectory(${CMAKE_CURRENT_LIST_DIR})

pico_enable_stdio_uart(app_main 1)
pico_add_extra_outputs(app_main)

# add url via pico_set_program_url
# example_auto_set_url(app_main)

