#pragma once

#include "color_structures.h"

#include <stddef.h>

// define_converter(basic_float_rgb2yuv);
// define_converter(basic_float_yuv2rgb);

void __attribute__(( ms_abi )) basic_float_rgb2yuv(const uint8_t *in, uint8_t *restrict out,
    size_t width, size_t height,
    ptrdiff_t in_stride, ptrdiff_t out_stride);

void __attribute__(( ms_abi )) basic_float_yuv2rgb(const uint8_t *in, uint8_t *restrict out,
    size_t width, size_t height,
    ptrdiff_t in_stride, ptrdiff_t out_stride);

void __attribute__(( ms_abi )) basic_fixed_rgb2yuv(const uint8_t *in, uint8_t *restrict out,
    size_t width, size_t height,
    ptrdiff_t in_stride, ptrdiff_t out_stride);

void __attribute__(( ms_abi )) basic_fixed_yuv2rgb(const uint8_t *in, uint8_t *restrict out,
    size_t width, size_t height,
    ptrdiff_t in_stride, ptrdiff_t out_stride);

void __attribute__(( ms_abi )) yuv2rgb_avx2(const uint8_t *in, uint8_t *restrict out,
    size_t width, size_t height,
    ptrdiff_t in_stride, ptrdiff_t out_stride);

void __attribute__(( ms_abi )) rgb2yuv_avx2(const uint8_t *in, uint8_t *restrict out,
    size_t width, size_t height,
    ptrdiff_t in_stride, ptrdiff_t out_stride);

void __attribute__(( ms_abi )) rgb2yuv_avx2_improved(const uint8_t *in, uint8_t *restrict out,
    size_t width, size_t height,
    ptrdiff_t in_stride, ptrdiff_t out_stride);

void __attribute__(( ms_abi )) yuv2rgb_avx2_improved(const uint8_t *in, uint8_t *restrict out,
    size_t width, size_t height,
    ptrdiff_t in_stride, ptrdiff_t out_stride);
