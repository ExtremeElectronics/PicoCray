add_executable(cray cray.c)

pico_enable_stdio_uart(cray 1)
pico_enable_stdio_usb(cray 1)

pico_add_extra_outputs(cray)

target_compile_options(cray PRIVATE -Wall)

target_link_libraries(cray i2c_slave pico_stdlib hardware_adc)


# enable usb output, disable uart output
pico_enable_stdio_usb(cray 1)


add_compile_definitions(PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE=1)
add_compile_definitions(PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE=0)

add_compile_definitions(
    TUD_OPT_RP2040_USB_DEVICE_ENUMERATION_FIX=1
)




