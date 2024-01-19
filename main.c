#include "converters.h"
#include "test_utils.h"
#include "tests.h"

#include <stdio.h>

int test_fast_correctness(const converter rgb2yuv_conv, const converter yuv2rgb_conv, const char* name) {
    printf("Start correctness tests: %s\n", name);

    int result = test_isomorphism(rgb2yuv_conv, yuv2rgb_conv, name);
    if (result != 0) goto DONE;
    printf("\n");

    result = test_sample(rgb2yuv_conv, yuv2rgb_conv, name);

    DONE:
    printf("Finish correctness tests: %s with result %d\n\n", name, result);
    return result;
}

int test_performance(const converter rgb2yuv_conv, const converter yuv2rgb_conv, const char* name) {
    printf("Start performance test: %s\n", name);
    test_pref_no_padding(rgb2yuv_conv, yuv2rgb_conv);
    printf("Finish performance test: %s\n\n", name);
    return 0;
}

int test_full_correctness(const converter rgb2yuv_conv, const converter yuv2rgb_conv, const char* name) {
    printf("Start full correctness tests: %s\n", name);

    int result = test_compare_with_model(rgb2yuv_conv, yuv2rgb_conv,
        basic_float_rgb2yuv, basic_float_yuv2rgb, name);

    printf("Finish full correctness tests: %s with result %d\n\n", name, result);

    return result;
}

int main() {
    converter rgb2yuv_check[] = {
        basic_fixed_rgb2yuv,
        rgb2yuv_avx2
    };
    converter yuv2rgb_check[] = {
        basic_fixed_yuv2rgb,
        yuv2rgb_avx2
    };
    const char* names[] = {
        "default fixed impl",
        "avx2"
    };

    for (size_t i = 0; i < sizeof(names) / sizeof(char*); i++) {
        test_fast_correctness(rgb2yuv_check[i], yuv2rgb_check[i], names[i]);
    }

    for (size_t i = 0; i < sizeof(names) / sizeof(char*); i++) {
        test_performance(rgb2yuv_check[i], yuv2rgb_check[i], names[i]);
    }

    for (size_t i = 0; i < sizeof(names) / sizeof(char*); i++) {
        test_full_correctness(rgb2yuv_check[i], yuv2rgb_check[i], names[i]);
    }
    return 0;
}
