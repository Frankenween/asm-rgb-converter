#include "color_structures.h"

#include <math.h>
#include <stdio.h>

uint8_t saturate(const double x) {
    const int rounded = round(x);
    if (rounded < 0) return 0;
    if (rounded > 255) return 255;
    return rounded;
}

void rgb2yuv_float(const rgb* src, yuv* dst) {
    dst->y  = saturate(0   + 0.299    * src->r + 0.587    * src->g + 0.114    * src->b);
    dst->cb = saturate(128 - 0.168736 * src->r - 0.331264 * src->g + 0.5      * src->b);
    dst->cr = saturate(128 + 0.5      * src->r - 0.418688 * src->g - 0.081312 * src->b);
    dst->dummy = 0;
}

void yuv2rgb_float(const yuv* src, rgb* dst) {
    dst->r = saturate(src->y                              + 1.402    * ((int)src->cr - 128));
    dst->g = saturate(src->y - 0.344136 * ((int)src->cb - 128) - 0.714136 * ((int)src->cr - 128));
    dst->b = saturate(src->y + 1.772    * ((int)src->cb - 128));
}

void rgb2yuv_fixed(const rgb* src, yuv* dst) {
    // Fixed point, 16 bit, 8 for fractional part

    // 0.299 = 77, 0.587 = 150, 0.114 = 29
    uint16_t y = 77 * src->r + 150 * src->g + 29 * src->b;

    // -0.168736 = -43, -0.331264 = -85, 0.5 = 128
    // 128 violates range, so closest value 127.99609375 was used
    uint16_t cb = 32767 - 43 * src->r - 85 * src->g + 128 * src->b;

    // 0.5 = 128, -0.418688 = -107, -0.081312 = -21
    uint16_t cr = 32767 + 128 * src->r - 107 * src->g - 21 * src->b;

    dst->y = y >> 8;
    dst->cb = cb >> 8;
    dst->cr = cr >> 8;
    dst->dummy = 0;
}

int saturate8_int(int x) {
    if (x < 0) return 0;
    if (x > 0xFFFF) return 0xFFFF;
    return x;
}

void yuv2rgb_fixed(const yuv* src, rgb* dst) {
    // 1 = 256, 1.402 = 359
    int r = src->y * 256 + 359 * ((int) src->cr - 128);

    // 1 = 256, 0.344136 = 88, 0.714136 = 183
    int g = 256 * src->y - 88 * ((int) src->cb - 128) - 183 * ((int) src->cr - 128);

    // 1 = 256, 1.772 = 454
    int b = 256 * src->y + 454 * ((int) src->cb - 128);

    r = saturate8_int(r);
    g = saturate8_int(g);
    b = saturate8_int(b);

    dst->r = r >> 8;
    dst->g = g >> 8;
    dst->b = b >> 8;
}