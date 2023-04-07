add_library(gfx
	gfx.c
)

target_include_directories(gfx PUBLIC
	.
)

target_link_libraries(gfx pico_stdlib)