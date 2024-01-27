global yuv2rgb_avx2_improved

section .rodata

align 32
YUV2RGB_MUL_YMM0:
    dw 64, -22, 0, 0, 64, -22, 0, 0, 64, -22, 0, 0, 64, -22, 0, 0

YUV2RGB_MUL_YMM1:
    dw 90, 64, 113, 0, 90, 64, 113, 0, 90, 64, 113, 0, 90, 64, 113, 0

YUV2RGB_MUL_YMM2:
    dw 0, -46, 64, 0, 0, -46, 64, 0, 0, -46, 64, 0, 0, -46, 64, 0

YUV2RGB_BIAS:
    %rep 4
    dw -90 * 128, 68 * 128, -113 * 128, 0
    %endrep

YUV2RGB_RGB_IN_SHUFFLE:
    %rep 2
    db 0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 128, 128, 128, 128
    %endrep

section .text

; MS ABI calling convention
; arg1(rcx) - in         :: const uint8_t*
; arg2(rdx) - out        :: uint8_t* restrict
; arg3(r8) - width      :: size_t
; arg4(r9) - height     :: size_t
; arg5 - in_stride  :: ptrdiff_tФ
; arg6 - out_stride :: ptrdiff_t
; return - void
yuv2rgb_avx2_improved:
    push rbx
    push r12
    push r13

    vpxor ymm3, ymm3, ymm3 ; ymm3 = 0

    START_ROW_PROCESSING_YUV2RGB:
    mov rax, r8 ; how many pixels in row left
    mov r10, rcx
    mov r11, rdx

    PROCESS_SIMD_YUV2RGB:
        cmp rax, 4 ; 6 * 3 = 18 > 16, so can just write xmm
        jb SMALL_WIDTH_READ_YUV2RGB ; too small width, not implemented


        ; ymm0 = [y cb cr x | y cb cr x | y cb cr x | y cb cr x]
        ; ymm1 = [cr y cb x | cr y cb x | cr y cb x | cr y cb x]
        ; ymm2 = [cb cr y x | cb cr y x | cb cr y x | cb cr y x]
        vmovdqu xmm0, [r10] ; read 16 bytes, need to unpack them
        RUN_YUV2RGB:
            vpmovzxbw ymm0, xmm0 ; zero-extend bytes to words
            ; reorder words
            vpshuflw ymm1, ymm0, 0b00010010
            vpshuflw ymm2, ymm0, 0b00001001
            ; now permute high parts in-place
            vpshufhw ymm1, ymm1, 0b00010010
            vpshufhw ymm2, ymm2, 0b00001001

            vpmullw ymm0, [rel YUV2RGB_MUL_YMM0]
            vpmullw ymm2, [rel YUV2RGB_MUL_YMM2]
            vpmullw ymm1, [rel YUV2RGB_MUL_YMM1]

            vpaddw ymm1, ymm1, ymm2
            vpaddw ymm1, ymm1, ymm0
            vpaddw ymm1, ymm1, [rel YUV2RGB_BIAS] ; add with saturation
            vpmaxsw ymm0, ymm1, ymm3 ; word = max(word, 0)
            vpsrlw ymm0, ymm0, 6 ; divide all by 64
            ; [r g b 0 | r g b 0 | r g b 0 | r g b 0]
            ; now we need to reorder words:

            vpshufb ymm0, ymm0, [rel YUV2RGB_RGB_IN_SHUFFLE]
             ;   [r g b r g b 0 0 | r g b r g b 0 0]
            vpackuswb ymm0, ymm0, ymm0 ; pack words to bytes
            ; now ymm0 is [(r0 g0 b0) (r1 g1 b1) 0 0 (r0 g0 b0) (r1 g1 b1) 0 0| (r2 g2 b2) (r3 g3 b3) 0 0 (r2 g2 b2) (r3 g3 b3) 0 0]
            vpermq ymm0, ymm0, 0b00001000
            ; now ymm0 is [(r0 g0 b0) (r1 g1 b1) 0 0 (r2 g2 b2) (r3 g3 b3) 0 0 | ...]
            ; shuffle xmm0 now
            vpshufb xmm0, xmm0, [rel YUV2RGB_RGB_IN_SHUFFLE]
            ; xmm0 is [(rgb) (rgb) (rgb) (rgb) 0000] now

        cmp rax, 6
        jb SMALL_WIDTH_WRITE_YUV2RGB
        vmovdqu [r11], xmm0 ; stored
        add r10, 4 * 4 ;
        add r11, 3 * 4
        sub rax, 4
        jmp PROCESS_SIMD_YUV2RGB

    FINISHED_ROW_YUV2RGB:
        add rcx, [rsp + 8 + 32 + 3 * 8] ; add in_stride
        add rdx, [rsp + 8 + 32 + 4 * 8] ; add out_stride
        dec r9 ; row processed, decrement now
        jnz START_ROW_PROCESSING_YUV2RGB

    pop r13
    pop r12
    pop rbx
    ret

    SMALL_WIDTH_READ_YUV2RGB:
        cmp rax, 0
        je FINISHED_ROW_YUV2RGB

        xor rbx, rbx
        xor r12, r12
        sub rsp, 16

        SMALL_COPY_TO_STACK_LOOP_YUV2RGB:
            mov r13d, [r10 + r12]
            mov [rsp + r12], r13d
            inc rbx
            add r12, 4 ; increment read pos
            cmp rbx, rax
            jne SMALL_COPY_TO_STACK_LOOP_YUV2RGB

        vmovdqu xmm0, [rsp]
        add rsp, 16
        jmp RUN_YUV2RGB

    SMALL_WIDTH_WRITE_YUV2RGB:
        sub rsp, 16
        xor rbx, rbx ; pixel index
        xor r12, r12
        vmovdqu [rsp], xmm0

        SMALL_COPY_TO_DST_LOOP_YUV2RGB:
            mov r13w, [rsp + r12]
            mov [r11], r13w
            mov r13b, [rsp + r12 + 2]
            mov [r11 + 2], r13b
            inc rbx
            add r12, 3
            add r11, 3 ; update dst index
            add r10, 4 ; we really read this pixel, so skip it
            cmp rbx, rax
            jne SMALL_COPY_TO_DST_LOOP_YUV2RGB

        xor rax, rax
        add rsp, 16
        jmp PROCESS_SIMD_YUV2RGB
