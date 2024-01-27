#include <stdio.h>
#include <x86intrin.h>

#include "tests.h"

// Return result of rdtscp-measured time, divided by number of pixels
// -1 if memory allocation failed
// Place test data in rgb section, even if yuv2rgb is tested
// In yuv place padding info
long double run_perf_test(const struct test_with_padding *test, const converter conv) {
    static const int TOTAL_RUNS = 20;
    long double eval_time = 0;

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

int test_perf_no_padding(const converter rgb2yuv_conv, const converter yuv2rgb_conv) {
    printf(" Start perf test for big width\n");

    long double rgb_worst_time = 0, yuv_worst_time = 0;
    size_t rgb2yuv_worst_width = 0, rgb2yuv_worst_height = 0, yuv2rgb_worst_width = 0, yuv2rgb_worst_height = 0;

    long double rgb2yuv_avg_time = 0, yuv2rgb_avg_time = 0;
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
            const long double rgb2yuv_time = run_perf_test(&test, rgb2yuv_conv);
            if (rgb2yuv_time < 0) {
                printf("  Failed rgb2yuv perf test %lux%lu\n", width, height);
                free(data);
                return -1;
            }
            const long double yuv2rgb_time = run_perf_test(&test, yuv2rgb_conv);
            if (yuv2rgb_time < 0) {
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

    printf("  Avg time:\n    RGB->YUV: %Lg\n    YUV->RGB: %Lg\n", rgb2yuv_avg_time, yuv2rgb_avg_time);
    printf("  Worst RGB->YUV time: %Lg on %lux%lu\n", rgb_worst_time, rgb2yuv_worst_height, rgb2yuv_worst_width);
    printf("  Worst YUV->RGB time: %Lg on %lux%lu\n", yuv_worst_time, yuv2rgb_worst_height, yuv2rgb_worst_width);
    printf(" Finish perf test for big width\n");

    return 0;
}

int test_perf_small_width(const converter rgb2yuv_conv, const converter yuv2rgb_conv) {
    printf(" Start perf test for small width\n");
    long double rgb_worst_time = 0, yuv_worst_time = 0;
    size_t rgb2yuv_worst_width = 0, rgb2yuv_worst_height = 0, yuv2rgb_worst_width = 0, yuv2rgb_worst_height = 0;

    long double rgb2yuv_avg_time = 0, yuv2rgb_avg_time = 0;
    for (size_t w_id = 0; w_id < PERF_SMALL_WIDTH_TEST_NUM; w_id++) {
            const size_t width = TEST_PERFORMANCE_SMALL_WIDTH[w_id];
            const size_t height = 7128;
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
            const long double rgb2yuv_time = run_perf_test(&test, rgb2yuv_conv);
            if (rgb2yuv_time < 0) {
                printf("  Failed rgb2yuv perf test %lux%lu\n", width, height);
                free(data);
                return -1;
            }
            const long double yuv2rgb_time = run_perf_test(&test, yuv2rgb_conv);
            if (yuv2rgb_time < 0) {
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
            printf("    Done %lux%lu: rgb2yuv %Lg, yuv2rgb %Lg\n", height, width, rgb2yuv_time, yuv2rgb_time);
    }
    rgb2yuv_avg_time /= PERF_TEST_NUM * PERF_TEST_NUM;
    yuv2rgb_avg_time /= PERF_TEST_NUM * PERF_TEST_NUM;

    printf("  Avg time:\n    RGB->YUV: %Lg\n    YUV->RGB: %Lg\n", rgb2yuv_avg_time, yuv2rgb_avg_time);
    printf("  Worst RGB->YUV time: %Lg on %lux%lu\n", rgb_worst_time, rgb2yuv_worst_height, rgb2yuv_worst_width);
    printf("  Worst YUV->RGB time: %Lg on %lux%lu\n", yuv_worst_time, yuv2rgb_worst_height, yuv2rgb_worst_width);
    printf(" Finish perf test for small width\n");

    return 0;
}