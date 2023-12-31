#pragma once

#include <stddef.h>
#include <stdint.h>

typedef void(*converter)(const uint8_t*, uint8_t *restrict,
    size_t, size_t,
    ptrdiff_t, ptrdiff_t) __attribute__(( ms_abi ));

// Contains array width, height and two arrays with rgb and yuv pixels, placed sequentially
// Pointers can be cast to (const rgb*) / (const yuv*)
struct test_cmp_answer {
    size_t width, height;
    const uint8_t *rgb_data;
    const uint8_t *yuv_data;
};

typedef void*(*allocator)(size_t);

// Create new test, based on the other test.
// Every row in rgb and yuv picture will be surrounded by specified number of bytes
// Memeory allocation is done via provided allocator function
struct test_cmp_answer make_bounds(const struct test_cmp_answer &base,
    size_t prefix_rgb, size_t suffix_rgb,
    size_t prefix_yuv, size_t suffix_yuv,
    allocator alloc);