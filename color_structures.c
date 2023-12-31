#include "color_structures.h"

#include <stdio.h>

void rgb2yuv_float(const rgb& src, yuv& dst) {
    dst.y  = 0   + 0.299    * src.r + 0.587    * src.g + 0.114    * src.b;
    dst.cb = 128 - 0.168736 * src.r - 0.331264 * src.g + 0.5      * src.b;
    dst.cr = 128 + 0.5      * src.r - 0.418688 * src.g - 0.081312 * src.b;
    dst.dummy = 0;
}

void yuv2rgb_float(const yuv& src, rgb& dst) {
    dst.r = src.y                             + 1.402    * (src.cr - 128);
    dst.g = src.y - 0.344136 * (src.cb - 128) - 0.714136 * (src.cr - 128);
    dst.b = src.y + 1.772    * (src.cb - 128);
}

void rgb2yuv_fixed(const rgb& src, yuv& dst) {
    printf("rgb -> yuv with fixed point is not implemented\n");
}

void yuv2rgb_fixed(const yuv& src, rgb& dst) {
    printf("yuv -> rgb with fixed point is not implemented\n");
}