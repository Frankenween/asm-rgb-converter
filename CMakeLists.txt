cmake_minimum_required(VERSION 3.27)
project(rgb_converter C ASM)
enable_language(ASM_NASM)

set(CMAKE_C_STANDARD 11)

set_property(SOURCE src/avx2_conv.asm PROPERTY LANGUAGE ASM_NASM)


add_executable(rgb_converter main.c
        color_structures.h
        color_structures.c
        converters.h
        test_utils.h
        test_utils.c
        src/float_cnvrt.c
        src/fixed_cnvrt.c
        src/avx2_conv.asm
)

target_include_directories(rgb_converter PRIVATE ${CMAKE_SOURCE_DIR})
# target_compile_options(rgb_converter PUBLIC -maccumulate-outgoing-args)
target_link_libraries(rgb_converter PRIVATE m)
