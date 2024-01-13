#pragma once

#include <stddef.h>
#include <stdint.h>

typedef void(*converter)(const uint8_t* in, uint8_t *restrict out,
                         size_t width, size_t height,
                         ptrdiff_t in_stride, ptrdiff_t out_stride) __attribute__(( ms_abi ));

// Contains array width, height and two arrays with rgb and yuv pixels, placed sequentially
// Pointers can be cast to (const rgb*) / (const yuv*)
struct test {
    size_t width, height;
    const uint8_t *rgb_data;
    const uint8_t *yuv_data;
};

struct test_with_padding {
    size_t width;
    size_t height;
    ptrdiff_t rgb_next_row_delta;
    const uint8_t *rgb_data;
    ptrdiff_t yuv_next_row_delta;
    const uint8_t *yuv_data;
};

size_t get_rgb_test_size(const struct test_with_padding* test);

size_t get_yuv_test_size(const struct test_with_padding* test);

uint8_t get_maximum_delta(const uint8_t *arr1, const uint8_t *arr2,
    size_t byte_width, size_t height, ptrdiff_t stride);

typedef void*(*allocator)(size_t);
typedef void(*mem_free)(void*);

const uint8_t RGB_FILL = 0x71;
const uint8_t YUV_FILL = 0x13;

// Create new test, based on the other test.
// Every row in rgb and yuv picture will be surrounded by specified number of bytes
// Memeory allocation is done via provided allocator function, freeing via provided function
// If failed to create, returns a structure with all zero fields.
struct test_with_padding make_bounds(const struct test *base,
        size_t prefix_rgb, size_t suffix_rgb,
        size_t prefix_yuv, size_t suffix_yuv,
        allocator alloc, mem_free freeer);

void free_test(struct test_with_padding *test, mem_free freeer);