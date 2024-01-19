#include <stdio.h>
#include <stdlib.h>

#include "tests.h"

// Apply converter c, then apply c_inv to the result. Check if result is the same
// Returns maximum absolute difference in pixel components, -1 in case of malloc error
// is_rgb is true, if c is rgb->yuv converter
int _check_isomorphism(const struct test_with_padding* test, const converter c, const converter c_inv) {
    uint8_t *rgb_place = malloc(get_rgb_test_size(test));
    if (rgb_place == 0) {
        goto RGB_MALLOC_FAIL;
    }
    uint8_t *yuv_place = malloc(get_yuv_test_size(test));
    if (yuv_place == 0) {
        goto YUV_MALLOC_FAIL;
    }
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
    uint8_t answer = get_maximum_delta(
        test->rgb_data, rgb_place,
        test->width * sizeof(rgb), test->height, test->rgb_next_row_delta);


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
    yuv got_worst = {0, 0, 0, 0};
    rgb compl_worst = {0, 0, 0};


    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 256; j++) {
            for (int k = 0; k < 256; k++) {
                const rgb chnls = {i, j, k};
                const rgb test_pixels[] = {chnls};
                const size_t TEST_SIZE = sizeof(test_pixels) / sizeof(rgb);
                struct test_with_padding t = {
                    .width = TEST_SIZE,
                    .height = 1,
                    .rgb_next_row_delta = TEST_SIZE * 3,
                    .rgb_data = (uint8_t*) test_pixels,
                    .yuv_next_row_delta = TEST_SIZE * 4,
                    .yuv_data = 0
                };
                const int rgb2yuv = _check_isomorphism(&t, rgb2yuv_conv, yuv2rgb_conv);
                if (rgb2yuv < 0) goto TEST_FAILED;

                if (max_rgb_delta < rgb2yuv) {
                    yuv yuv_dummy[TEST_SIZE];
                    rgb rgb_dummy[TEST_SIZE];

                    max_rgb_delta = rgb2yuv;
                    worst_rgb.r = i;
                    worst_rgb.g = j;
                    worst_rgb.b = k;
                    rgb2yuv_conv((uint8_t*) test_pixels, (uint8_t*) yuv_dummy, TEST_SIZE, 1,
                        TEST_SIZE * 3, TEST_SIZE * 4);
                    yuv2rgb_conv((uint8_t*) yuv_dummy, (uint8_t*) rgb_dummy, TEST_SIZE, 1,
                        TEST_SIZE * 4, TEST_SIZE * 3);
                    got_worst = yuv_dummy[0];
                    compl_worst = rgb_dummy[0];
                }
            }
        }
    }
    printf("    Max rgb channel delta %d\n", max_rgb_delta);
    printf("    Worst rgb: (%d, %d, %d)\n", worst_rgb.r, worst_rgb.g, worst_rgb.b);
    printf("    RGB -> YUV: (%d, %d, %d)\n", got_worst.y, got_worst.cb, got_worst.cr);
    printf("    YUV -> RGB: (%d, %d, %d)\n", compl_worst.r, compl_worst.g, compl_worst.b);
    printf("  Finish all colors check: %s\n", name);
    return 0;

    TEST_FAILED:
    printf("  Malloc failed, finishing : %s\n", name);
    return -1;
}
