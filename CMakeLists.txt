cmake_minimum_required(VERSION 3.27)
project(rgb_converter C ASM)
enable_language(ASM_NASM)

set(CMAKE_C_STANDARD 11)

add_executable(rgb_converter main.c
        color_structures.h
        color_structures.c
        converters.h
        src/float_cnvrt.c
        test_utils.h)

target_include_directories(rgb_converter PUBLIC ${CMAKE_SOURCE_DIR})
