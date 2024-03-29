#include "converters.h"

void __attribute__(( ms_abi )) basic_fixed_rgb2yuv(const uint8_t *in, uint8_t *restrict out,
        size_t width, size_t height,
        ptrdiff_t in_stride, ptrdiff_t out_stride) {
    for (size_t row = 0; row < height; row++) {
        const rgb *row_pixels_rgb = (const rgb*)(in + in_stride * row);
        yuv *row_pixels_yuv = (yuv*)(out + out_stride * row);
        for (size_t p = 0; p < width; p++) {
            rgb2yuv_fixed7(&row_pixels_rgb[p], &row_pixels_yuv[p]);
        }
    }
}

void __attribute__(( ms_abi )) basic_fixed_yuv2rgb(const uint8_t *in, uint8_t *restrict out,
        size_t width, size_t height,
        ptrdiff_t in_stride, ptrdiff_t out_stride) {
    for (size_t row = 0; row < height; row++) {
        const yuv *row_pixels_yuv = (const yuv*)(in + in_stride * row);
        rgb *row_pixels_rgb = (rgb*)(out + out_stride * row);
        for (size_t p = 0; p < width; p++) {
            yuv2rgb_fixed6(&row_pixels_yuv[p], &row_pixels_rgb[p]);
        }
    }
}