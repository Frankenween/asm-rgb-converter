#include <stdio.h>

#include "tests.h"

// Long test, used to compare converter with model solutions
// Each row is filled with every possible pixel combination several times, so SIMD processing is done
int test_compare_with_model(
    const converter rgb2yuv_conv, const converter yuv2rgb_conv,
    const converter model_rgb2yuv, const converter model_yuv2rgb,
    const char* name) {
    static const int TOLERANCE = 2;

    int result = 0;
    printf("  Start compare with model converters: %s\n", name);

    for (int len_id = 0; len_id < CORRECTNESS_TEST_NUM; len_id++) {
        const size_t len = TEST_CORRECTNESS[len_id];
        rgb rgb_in[len];
        yuv yuv_in[len];
        printf("   Starting len %lu\n", len);
        for (int i = 0; i < 256; i++) {
            for (int j = 0; j < 256; j++) {
                for (int k = 0; k < 256; k++) {
                    const rgb rgb_pix = {i, j, k};
                    const yuv yuv_pix = {i, j, k, 0};

                    rgb conv_rgb_out[len], model_rgb_out[len];
                    yuv conv_yuv_out[len], model_yuv_out[len];

                    for (int pixel = 0; pixel < len; pixel++) {
                        rgb_in[pixel] = rgb_pix;
                        yuv_in[pixel] = yuv_pix;
                    }
                    // Run rgb2yuv converters
                    rgb2yuv_conv(
                        (uint8_t*) rgb_in, (uint8_t*)conv_yuv_out,
                        len, 1,
                        sizeof(rgb) * len, sizeof(yuv) * len
                    );
                    model_rgb2yuv(
                        (uint8_t*) rgb_in, (uint8_t*)model_yuv_out,
                        len, 1,
                        sizeof(rgb) * len, sizeof(yuv) * len
                    );
                    // Run yuv2rgb converters
                    yuv2rgb_conv(
                        (uint8_t*) yuv_in, (uint8_t*)conv_rgb_out,
                        len, 1,
                        sizeof(yuv) * len, sizeof(rgb) * len
                    );
                    model_yuv2rgb(
                        (uint8_t*) yuv_in, (uint8_t*)model_rgb_out,
                        len, 1,
                        sizeof(yuv) * len, sizeof(rgb) * len
                    );

                    for (size_t pixel = 0; pixel < len; pixel++) {
                        const uint8_t rgb_delta = get_rgb_delta(conv_rgb_out[pixel], model_rgb_out[pixel]);
                        uint8_t yuv_delta = get_yuv_delta(conv_yuv_out[pixel], model_yuv_out[pixel]);

                        if (rgb_delta > TOLERANCE) {
                            printf("    Failed yuv2rgb converter at pixel %lu, delta is %d\n", pixel, rgb_delta);
                            printf("    yuv = (%d, %d, %d), model rgb = (%d, %d, %d), got rgb = (%d, %d, %d)\n\n",
                                i, j, k,
                                model_rgb_out[pixel].r, model_rgb_out[pixel].g, model_rgb_out[pixel].b,
                                conv_rgb_out[pixel].r, conv_rgb_out[pixel].g, conv_rgb_out[pixel].b);
                            result = 1;
                            goto DONE;
                        }
                        if (yuv_delta > TOLERANCE) {
                            printf("    Failed rgb2yuv converter at pixel %lu, delta is %d\n", pixel, yuv_delta);
                            printf("    rgb = (%d, %d, %d), model yuv = (%d, %d, %d), got yuv = (%d, %d, %d)\n\n",
                                i, j, k,
                                model_yuv_out[pixel].y, model_yuv_out[pixel].cb, model_yuv_out[pixel].cr,
                                conv_yuv_out[pixel].y, conv_yuv_out[pixel].cb, conv_yuv_out[pixel].cr);
                            result = 1;
                            goto DONE;
                        }
                    }
                }
            }
        }

    }
    DONE:
    printf("  Finish compare with model converters: %s. Result is %d\n", name, result);
    return result;
}