cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

# We also need PICO EXTRAS
include(pico_extras_import.cmake)

project(shawn_pico C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(ssd1306)
add_subdirectory(ili9341)
add_subdirectory(kilo)
add_subdirectory(memory)
add_subdirectory(ribbit)
add_subdirectory(vga)
add_subdirectory(textmode)
add_subdirectory(coco4)
add_subdirectory(cpu6809)
add_subdirectory(cpu6502)
