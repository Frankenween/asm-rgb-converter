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
    printf("rgb -> yuv with fixed point is not implemented\n");
}

void yuv2rgb_fixed(const yuv* src, rgb* dst) {
    printf("yuv -> rgb with fixed point is not implemented\n");
}