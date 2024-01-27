#pragma once

#include "test_utils.h"

static const size_t TEST_CORRECTNESS[] = {
    1, 2, 3, 4, 5, 6, 7, 8, 15, 16,
    31, 32, 102, 105
};

static const size_t TEST_PERFORMANCE[] = {
    1023, 4071, 7198
};

static const size_t TEST_PERFORMANCE_SMALL_WIDTH[] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17
};

static const size_t PADDINGS[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 31, 32, 33, 49
};

static const size_t PERF_PADDINGS[] = {
    0, 1, 4, 5
};

static const size_t CORRECTNESS_TEST_NUM = sizeof(TEST_CORRECTNESS) / sizeof(size_t);
static const size_t PERF_TEST_NUM = sizeof(TEST_PERFORMANCE) / sizeof(size_t);
static const size_t PERF_SMALL_WIDTH_TEST_NUM = sizeof(TEST_PERFORMANCE_SMALL_WIDTH) / sizeof(size_t);
static const size_t PADDINGS_NUM = sizeof(PADDINGS) / sizeof(size_t);
static const size_t PERF_PADDINGS_NUM = sizeof(PERF_PADDINGS) / sizeof(size_t);

static const uint8_t sample_rgb[] = {
    15, 70, 44,      230, 151, 94,   73, 18, 194,
    83, 13, 211,     128, 0, 39,     18, 7, 199,
    228, 170, 134,   13, 37, 88,     14, 14, 255,
    82, 11, 179,     255, 100, 29,   19, 254, 11
};

static const uint8_t sample_yuv[] = {
    51, 124, 103, 0,   168, 86, 172, 0,   55, 207, 141, 0,
    57, 215, 147, 0,   43, 126, 189, 0,   32, 222, 118, 0,
    183, 100, 160, 0,   36, 158, 112, 0,   41, 248, 108, 0,
    51, 200, 150, 0,   138, 66, 211, 0,    156, 46, 30, 0
};

static const struct test sample_test = {
    .width = 3,
    .height = 4,
    .rgb_data = sample_rgb,
    .yuv_data = sample_yuv
};

int test_isomorphism(converter rgb2yuv_conv, converter yuv2rgb_conv, const char* name);

int test_sample(converter rgb2yuv_conv, converter yuv2rgb_conv, const char *name);

int test_compare_with_model(
    converter rgb2yuv_conv, converter yuv2rgb_conv,
    converter model_rgb2yuv, converter model_yuv2rgb,
    const char* name);

int test_perf_no_padding(converter rgb2yuv_conv, converter yuv2rgb_conv);

int test_perf_small_width(converter rgb2yuv_conv, converter yuv2rgb_conv);
