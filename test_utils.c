#include "test_utils.h"
#include "color_structures.h"

#include <stdlib.h>
#include <string.h>

size_t get_rgb_test_size(const struct test_with_padding* test) {
    return abs(test->rgb_next_row_delta) * test->height;
}

size_t get_yuv_test_size(const struct test_with_padding* test) {
    return abs(test->yuv_next_row_delta) * test->height;
}

uint8_t get_maximum_delta(const uint8_t *arr1, const uint8_t *arr2,
    const size_t byte_width, const size_t height, const ptrdiff_t stride) {
    uint8_t ans = 0;
    for (size_t r = 0; r < height; r++) {
        for (size_t i = 0; i < byte_width; i++) {
            const uint8_t delta = arr1[i] < arr2[i] ? arr2[i] - arr1[i] : arr1[i] - arr2[i];
            ans = ans < delta ? delta : ans;
        }
        arr1 += stride;
        arr2 += stride;
    }
    return ans;
}

struct test_with_padding make_bounds(const struct test *base,
        const size_t prefix_rgb, const size_t suffix_rgb,
        const size_t prefix_yuv, const size_t suffix_yuv,
        const allocator alloc, const mem_free freeer) {
    const size_t rgb_new_size = (prefix_rgb + suffix_rgb + base->width * sizeof(rgb)) * base->height;
    const size_t yuv_new_size = (prefix_yuv + suffix_yuv + base->width * sizeof(yuv)) * base->height;

    struct test_with_padding new_test;
    new_test.width = base->width;
    new_test.height = base->height;
    new_test.rgb_data = 0;
    new_test.rgb_next_row_delta = 0;
    new_test.yuv_data = 0;
    new_test.yuv_next_row_delta = 0;

    uint8_t* rgb_data = alloc(rgb_new_size);
    if (rgb_data == 0) {
        goto FAILED_NO_ALLOC;
    }
    uint8_t *yuv_data = alloc(yuv_new_size);
    if (yuv_data == 0) {
        goto FAILED_RGB_ALLOCATED;
    }
    memset(rgb_data, 0x7A, rgb_new_size);
    memset(yuv_data, 0xD5, yuv_new_size);
    for (size_t r = 0; r < base->height; r++) {
        uint8_t *start_rgb_row = rgb_data + (sizeof(rgb) * base->width + prefix_rgb + suffix_rgb) * r;
        uint8_t *start_yuv_row = yuv_data + (sizeof(yuv) * base->width + prefix_yuv + suffix_yuv) * r;
        memcpy(start_rgb_row, base->rgb_data + (sizeof(rgb) * base->width) * r, sizeof(rgb) * base->width);
        memcpy(start_yuv_row, base->yuv_data + (sizeof(yuv) * base->width) * r, sizeof(yuv) * base->width);
    }
    new_test.rgb_data = rgb_data;
    new_test.yuv_data = yuv_data;
    new_test.rgb_next_row_delta = prefix_rgb + suffix_rgb + sizeof(rgb) * base->width;
    new_test.yuv_next_row_delta = prefix_yuv + suffix_yuv + sizeof(yuv) * base->width;
    return new_test;

    FAILED_RGB_ALLOCATED:
    freeer(rgb_data);

    FAILED_NO_ALLOC:
    new_test.width = new_test.height = 0;
    return new_test;
}

void free_test(struct test_with_padding *test, const mem_free freeer) {
    freeer((void*)test->rgb_data);
    freeer((void*)test->yuv_data);
    test->rgb_data = 0;
    test->yuv_data = 0;
}
