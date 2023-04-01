add_library(i2c_slave INTERFACE)

target_include_directories(i2c_slave
    INTERFACE
    ./include)

target_sources(i2c_slave
    INTERFACE
    i2c_slave.c
)

target_link_libraries(i2c_slave
    INTERFACE
    hardware_i2c
    hardware_irq
)
