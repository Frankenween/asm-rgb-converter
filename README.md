# Color format converter
x86 NASM and C implementations of RGB to YCbCr and YCbCr to RGB converters.

YCbCr pixels have JPEG format(ITU-T T.871) and have one dummy channel, which can be used for transparency.

- Assembler implementations use SIMD instructions and require AVX2 extension
- Calling convention is Fastcall64
- Fixed-point 16-bit numbers are used instead of floating-point ones to increase performance.

These functions should not be used when image width is small 
because code for converting last pixels in row is not properly optimized.

# Tests
Test environment and some tests for checking converters' correctness and performance were implemented.

- Test with row paddings and small sample test
- Test, which checks that two converters are complement
- Big test, which checks all possible pixels and some small and big rows. 
Results are compared with model converter, which uses float values
- Performance test, which runs converters multiple times on different big random images.
Performance is measured via `rdtscp` function, result is divided by number of pixels, so it shows average "time" for processing a pixel