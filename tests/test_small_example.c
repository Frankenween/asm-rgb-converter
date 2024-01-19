#include <stdio.h>
#include <stdlib.h>

#include "tests.h"

// Check if converters pass sample test. They are allowed to have absolute mistake equal to 1
int test_sample(const converter rgb2yuv_conv, const converter yuv2rgb_conv, const char *name) {
    static const int DELTA_TOLERANCE = 3;
    printf("  Start sample test: %s\n", name);

    for (size_t pref_rgb_id = 0; pref_rgb_id < PADDINGS_NUM; pref_rgb_id++) {
        for (size_t suff_rgb_id = 0; suff_rgb_id < PADDINGS_NUM; suff_rgb_id++) {
            for (size_t pref_yuv_id = 0; pref_yuv_id < PADDINGS_NUM; pref_yuv_id++) {
                for (size_t suff_yuv_id = 0; suff_yuv_id < PADDINGS_NUM; suff_yuv_id++) {
                    const size_t pref_rgb = PADDINGS[pref_rgb_id];
                    const size_t pref_yuv = PADDINGS[pref_yuv_id];
                    const size_t suff_rgb = PADDINGS[suff_rgb_id];
                    const size_t suff_yuv = PADDINGS[suff_yuv_id];
                    struct test_with_padding padded = make_bounds(&sample_test,
                        pref_rgb, suff_rgb, pref_yuv, suff_yuv,
                        malloc, free);
                    if (padded.width == 0) {
                        printf("    Test allocation failed\n");
                        goto FAIL_TEST_ALLOC;
                    }

                    uint8_t *rgb_place = malloc(get_rgb_test_size(&padded));
                    if (rgb_place == 0) {
                        goto FAIL_RGB;
                    }
                    uint8_t *yuv_place = malloc(get_yuv_test_size(&padded));
                    if (yuv_place == 0) {
                        goto FAIL_YUV;
                    }

                    rgb2yuv_conv(padded.rgb_data, yuv_place,
                        padded.width, padded.height,
                        padded.rgb_next_row_delta, padded.yuv_next_row_delta);

                    yuv2rgb_conv(padded.yuv_data, rgb_place,
                        padded.width, padded.height,
                        padded.yuv_next_row_delta, padded.rgb_next_row_delta);

                    const int yuv_delta = get_maximum_delta(
                        padded.yuv_data, yuv_place,
                        sizeof(yuv) * padded.width, padded.height, padded.yuv_next_row_delta);
                    const int rgb_delta = get_maximum_delta(
                        padded.rgb_data, rgb_place,
                        sizeof(rgb) * padded.width, padded.height, padded.rgb_next_row_delta);
                    if (yuv_delta > DELTA_TOLERANCE || rgb_delta > DELTA_TOLERANCE) {
                        goto FAIL_TEST;
                    }
                    free(yuv_place);
                    free(rgb_place);
                    free_test(&padded, free);
                    continue;

                    FAIL_TEST:
                    printf("    Test [ %lu rgb_row %lu ], [ %lu yuv_row %lu ] failed: ",
                        pref_rgb, suff_rgb, pref_yuv, suff_yuv);
                    printf("rgb delta is %d, yuv delta is %d\n", rgb_delta, yuv_delta);
                    free(yuv_place);
                    FAIL_YUV:
                    free(rgb_place);
                    FAIL_RGB:
                    free_test(&padded, free);
                    FAIL_TEST_ALLOC:
                    return -1;
                }
            }
        }
    }

    DONE:
    printf("  Finish sample test: %s. OK\n", name);
    return 0;
}