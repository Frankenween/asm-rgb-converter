#include "converters.h"
#include "test_utils.h"

#include <stdio.h>
#include <stdlib.h>


const size_t TEST_CORRECTNESS[] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    31, 32, 33, 91, 100, 101, 102, 103, 104, 105
};

const size_t TEST_PERFORMANCE[] = {
    1023, 1024, 1025, 4071, 4096, 5018, 7198
};

const size_t PADDINGS[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 31, 32, 33, 49
};

const size_t PADDINGS_NUM = sizeof(PADDINGS) / sizeof(size_t);

uint8_t sample_rgb[] = {
    15, 70, 44,      230, 151, 94,   73, 18, 194,
    83, 13, 211,     128, 0, 39,     18, 7, 199,
    228, 170, 134,   13, 37, 88,     14, 14, 255,
    82, 11, 179,     255, 100, 29,   19, 254, 11
};

uint8_t sample_yuv[] = {
    51, 124, 103, 0,   168, 86, 172, 0,   55, 207, 141, 0,
    57, 215, 1470, 0,   43, 126, 189, 0,   32, 222, 118, 0,
    183, 100, 160, 0,   36, 158, 112, 0,   41, 248, 108, 0,
    51, 200, 150, 0,   138, 66, 211, 0,    156, 46, 30, 0,
};

struct test sample_test = {
    .width = 3,
    .height = 4,
    .rgb_data = sample_rgb,
    .yuv_data = sample_yuv
};

// Apply converter c, then apply c_inv to the result. Check if result is the same
// Returns maximum absolute difference in pixel components, -1 in case of malloc error
// is_rgb is true, if c is rgb->yuv converter
int _check_isomorphism(const struct test_with_padding* test, const converter c, const converter c_inv, const int is_rgb) {
    uint8_t *rgb_place = malloc(get_rgb_test_size(test));
    if (rgb_place == 0) {
        goto RGB_MALLOC_FAIL;
    }
    uint8_t *yuv_place = malloc(get_yuv_test_size(test));
    if (yuv_place == 0) {
        goto YUV_MALLOC_FAIL;
    }
    uint8_t answer;
    if (is_rgb) {
        c(
            test->rgb_data, yuv_place,
            test->width, test->height,
            test->rgb_next_row_delta, test->yuv_next_row_delta
        );
        c_inv(
            yuv_place, rgb_place,
            test->width, test->height,
            test->yuv_next_row_delta, test->rgb_next_row_delta
        );
        answer = get_maximum_delta(
            test->rgb_data, rgb_place,
            test->width * sizeof(rgb), test->height, test->rgb_next_row_delta);
    } else {
        c(
            test->yuv_data, rgb_place,
            test->width, test->height,
            test->yuv_next_row_delta, test->rgb_next_row_delta
        );
        c_inv(
            rgb_place, yuv_place,
            test->width, test->height,
            test->rgb_next_row_delta, test->yuv_next_row_delta
        );
        answer = get_maximum_delta(
            test->rgb_data, rgb_place,
            test->width * sizeof(yuv), test->height, test->yuv_next_row_delta);

    }

    free(yuv_place);
    free(rgb_place);
    return answer;

    YUV_MALLOC_FAIL:
    free(rgb_place);

    RGB_MALLOC_FAIL:
    return -1;
}

// Test if two converters are complement
int test_isomorphism(const converter rgb2yuv_conv, const converter yuv2rgb_conv, const char* name) {
    printf("  Starts all colors isomorphism check : %s\n", name);
    uint8_t max_rgb_delta = 0;
    rgb worst_rgb = {0, 0, 0};
    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 256; j++) {
            for (int k = 0; k < 256; k++) {
                uint8_t chnls[] = {i, j, k, 0};
                struct test t = {
                    .width = 1,
                    .height = 1,
                    .rgb_data = chnls,
                    .yuv_data = chnls
                };
                struct test_with_padding padded_test = make_bounds(&t,
                    0, 0, 0, 0,
                    malloc, free);
                const int rgb2yuv = _check_isomorphism(&padded_test, rgb2yuv_conv, yuv2rgb_conv, 1);
                free_test(&padded_test, free);
                if (rgb2yuv < 0) goto TEST_FAILED;

                if (max_rgb_delta < rgb2yuv) {
                    max_rgb_delta = rgb2yuv;
                    worst_rgb.r = i;
                    worst_rgb.g = j;
                    worst_rgb.b = k;
                }
            }
        }
    }
    printf("    Max rgb channel delta %d\n", max_rgb_delta);
    printf("    Worst rgb: (%d, %d, %d)\n", worst_rgb.r, worst_rgb.g, worst_rgb.b);
    printf("  Finish all colors check: %s\n", name);
    return 0;

    TEST_FAILED:
    printf("  Malloc failed, finishing : %s\n", name);
    return -1;
}

int test_no_modify_padding(const converter rgb2yuv_conv, const converter yuv2rgb_conv, const char *name) {
    printf("  Start padding test: %s\n", name);
    printf("    NOT IMPLEMENTED\n");
    goto DONE;

    for (size_t pref_rgb_id = 0; pref_rgb_id < PADDINGS_NUM; pref_rgb_id++) {
        for (size_t suff_rgb_id = 0; suff_rgb_id < PADDINGS_NUM; suff_rgb_id++) {
            for (size_t pref_yuv_id = 0; pref_yuv_id < PADDINGS_NUM; pref_yuv_id++) {
                for (size_t suff_yuv_id = 0; suff_yuv_id < PADDINGS_NUM; suff_yuv_id++) {
                    const size_t pref_rgb = PADDINGS[pref_rgb_id];
                    const size_t pref_yuv = PADDINGS[pref_yuv_id];
                    const size_t suff_rgb = PADDINGS[suff_rgb_id];
                    const size_t suff_yuv = PADDINGS[suff_yuv_id];
                    const struct test_with_padding padded = make_bounds(&sample_test,
                        pref_rgb, suff_rgb, pref_yuv, suff_yuv,
                        malloc, free);
                    if (padded.width == 0) return -1; // failed to alloc

                }
            }
        }
    }
    DONE:
    printf("  Finish padding test: %s\n", name);
    return 0;
}

int test_sample(const converter rgb2yuv_conv, const converter yuv2rgb_conv, const char *name) {
    printf(" Start sample test")
}

int test_correctness(const converter rgb2yuv_conv, const converter yuv2rgb_conv, const char* name) {
    int result = 0;
    printf("Start correctness: %s\n", name);

    result = test_isomorphism(rgb2yuv_conv, yuv2rgb_conv, name);
    if (result != 0) goto DONE;

    DONE:
    printf("Finish correctness: %s\n", name);
    return result;
}

int main() {
    test_correctness(basic_float_rgb2yuv, basic_float_yuv2rgb, "default float impl");
    return 0;
}
