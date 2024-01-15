#pragma once

#include <stdint.h>

struct rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct yuv {
    uint8_t y;
    uint8_t cb;
    uint8_t cr;
    uint8_t dummy;
};

typedef struct rgb rgb;
typedef struct yuv yuv;

void rgb2yuv_float(const rgb* src, yuv* dst);

void yuv2rgb_float(const yuv* src, rgb* dst);

void rgb2yuv_fixed(const rgb* src, yuv* dst);

void yuv2rgb_fixed(const yuv* src, rgb* dst);

void rgb2yuv_fixed7(const rgb* src, yuv* dst);

void yuv2rgb_fixed6(const yuv* src, rgb* dst);

#define define_converter(name) void __attribute__(( ms_abi )) ##name(const uint8_t *in, uint8_t *restrict out, \
        size_t width, size_t height, \
        ptrdiff_t in_stride, ptrdiff_t out_stride)
