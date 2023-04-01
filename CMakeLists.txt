cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(i2c_examples C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(i2c_slave)
add_subdirectory(picocray)

# enable usb output, disable uart output
pico_enable_stdio_usb(cray 1)


add_compile_definitions(PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE=1)
add_compile_definitions(PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE=0)

add_compile_definitions(
    TUD_OPT_RP2040_USB_DEVICE_ENUMERATION_FIX=1
)

