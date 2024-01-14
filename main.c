#include "converters.h"
#include "test_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <x86intrin.h>


const size_t TEST_CORRECTNESS[] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    31, 32, 33, 91, 100, 101, 102, 103, 104, 105
};

const size_t TEST_PERFORMANCE[] = {
    1023, 4071, 7198
};

const size_t PADDINGS[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 31, 32, 33, 49
};

const size_t PERF_PADDINGS[] = {
    0, 1, 4, 5
};

const size_t PERF_TEST_NUM = sizeof(TEST_PERFORMANCE) / sizeof(size_t);
const size_t PADDINGS_NUM = sizeof(PADDINGS) / sizeof(size_t);
const size_t PERF_PADDINGS_NUM = sizeof(PERF_PADDINGS) / sizeof(size_t);

uint8_t sample_rgb[] = {
    15, 70, 44,      230, 151, 94,   73, 18, 194,
    83, 13, 211,     128, 0, 39,     18, 7, 199,
    228, 170, 134,   13, 37, 88,     14, 14, 255,
    82, 11, 179,     255, 100, 29,   19, 254, 11
};

uint8_t sample_yuv[] = {
    51, 124, 103, 0,   168, 86, 172, 0,   55, 207, 141, 0,
    57, 215, 147, 0,   43, 126, 189, 0,   32, 222, 118, 0,
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

// Check if converters pass sample test. They are allowed to have absolute mistake equal to 1
int test_sample(const converter rgb2yuv_conv, const converter yuv2rgb_conv, const char *name) {
    static const int DELTA_TOLERANCE = 2;
    printf("  Start padding test: %s\n", name);

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
    printf("  Finish padding test: %s. OK\n", name);
    return 0;
}

int test_correctness(const converter rgb2yuv_conv, const converter yuv2rgb_conv, const char* name) {
    int result = 0;
    printf("Start correctness: %s\n", name);

    result = test_isomorphism(rgb2yuv_conv, yuv2rgb_conv, name);
    if (result != 0) goto DONE;
    printf("\n");

    result = test_sample(rgb2yuv_conv, yuv2rgb_conv, name);

    DONE:
    printf("Finish correctness: %s with result %d\n", name, result);
    return result;
}

// Return result of rdtscp-measured time, divided by number of pixels
// -1 if memory allocation failed
// Place test data in rgb section, even if yuv2rgb is tested
// In yuv place padding info
uint64_t run_perf_test(struct test_with_padding *test, converter conv) {
    static const int TOTAL_RUNS = 20;
    uint64_t eval_time = 0;

    uint8_t *out = malloc(get_yuv_test_size(test));
    if (out == 0) {
        goto MALLOC_FAILED;
    }
    unsigned int dummy;
    for (int attempt = 0; attempt < TOTAL_RUNS; attempt++) {
        uint64_t t1 = __rdtscp(&dummy);
        conv(test->rgb_data, out,
            test->width, test->height,
            test->rgb_next_row_delta, test->yuv_next_row_delta);
        uint64_t t2 = __rdtscp(&dummy);
        eval_time += t2 - t1;
    }
    eval_time /= TOTAL_RUNS;
    eval_time /= test->width * test->height;

    free(out);
    return eval_time;

    MALLOC_FAILED:
    return -1;
}

int test_pref_no_padding(const converter rgb2yuv_conv, const converter yuv2rgb_conv) {
    size_t rgb_worst_time = 0, yuv_worst_time = 0;
    size_t rgb2yuv_worst_width = 0, rgb2yuv_worst_height = 0, yuv2rgb_worst_width = 0, yuv2rgb_worst_height = 0;

    size_t rgb2yuv_avg_time = 0, yuv2rgb_avg_time = 0;
    for (size_t w_id = 0; w_id < PERF_TEST_NUM; w_id++) {
        for (size_t h_id = 0; h_id < PERF_TEST_NUM; h_id++) {
            const size_t width = TEST_PERFORMANCE[w_id];
            const size_t height = TEST_PERFORMANCE[h_id];
            uint8_t *data = malloc(width * height * sizeof(yuv));
            if (data == 0) {
                printf("  Failed to allocate array for test %lux%lu\n", width, height);
                return -1;
            }
            struct test_with_padding test = {
                .width =  width,
                .height = height,
                .rgb_data = data,
                .rgb_next_row_delta = width * sizeof(rgb), // test rgb first
                .yuv_next_row_delta = width * sizeof(yuv),
                .yuv_data = 0
            };
            const uint64_t rgb2yuv_time = run_perf_test(&test, rgb2yuv_conv);
            if (rgb2yuv_time == -1) {
                printf("  Failed rgb2yuv perf test %lux%lu\n", width, height);
                free(data);
                return -1;
            }
            const uint64_t yuv2rgb_time = run_perf_test(&test, yuv2rgb_conv);
            if (yuv2rgb_time == -1) {
                printf("  Failed yuv2rgb perf test %lux%lu\n", width, height);
                free(data);
                return -1;
            }
            free(data);

            rgb2yuv_avg_time += rgb2yuv_time;
            yuv2rgb_avg_time += yuv2rgb_time;

            if (rgb_worst_time < rgb2yuv_time) {
                rgb_worst_time = rgb2yuv_time;
                rgb2yuv_worst_width = width;
                rgb2yuv_worst_height = height;
            }
            if (yuv_worst_time < yuv2rgb_time) {
                yuv_worst_time = yuv2rgb_time;
                yuv2rgb_worst_width = width;
                yuv2rgb_worst_height = height;
            }
            printf("    Done %lux%lu\n", height, width);
        }
    }
    rgb2yuv_avg_time /= PERF_TEST_NUM * PERF_TEST_NUM;
    yuv2rgb_avg_time /= PERF_TEST_NUM * PERF_TEST_NUM;

    printf("  Avg time:\n    RGB->YUV: %lu\n    YUV->RGB: %lu\n", rgb2yuv_avg_time, yuv2rgb_avg_time);
    printf("  Worst RGB->YUV time: %lu on %lux%lu\n", rgb_worst_time, rgb2yuv_worst_height, rgb2yuv_worst_width);
    printf("  Worst YUV->RGB time: %lu on %lux%lu\n", yuv_worst_time, yuv2rgb_worst_height, yuv2rgb_worst_width);
    return 0;
}

int test_performance(const converter rgb2yuv_conv, const converter yuv2rgb_conv, const char* name) {
    printf("Start performance test: %s\n", name);
    test_pref_no_padding(rgb2yuv_conv, yuv2rgb_conv);
    printf("Finished performance test: %s\n", name);
    return 0;
}

int main() {
    // test_correctness(basic_float_rgb2yuv, basic_float_yuv2rgb, "default float impl");
    // printf("\n");
    // test_performance(basic_float_rgb2yuv, basic_float_yuv2rgb, "default float impl");
    // printf("\n\n");

    test_correctness(basic_fixed_rgb2yuv, basic_fixed_yuv2rgb, "default float impl");
    printf("\n");
    test_performance(basic_fixed_rgb2yuv, basic_fixed_yuv2rgb, "default float impl");
    return 0;
}
