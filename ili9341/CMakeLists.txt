add_library(ili9341
	ili9341.c
)

target_include_directories(ili9341 PUBLIC
	.
)

target_link_libraries(ili9341 pico_stdlib hardware_spi hardware_dma)