; Removed redundant alignment statements
; Removed scalar code 

global rgb2yuv_avx2_improved

%define SIMD_SAVE_REGISTERS 4

section .rodata

align 32
RGB2YUV_YMM0_MUL:
    %rep 4
    dw 77, -85, -21, 0
    %endrep

; align 32
RGB2YUV_YMM1_MUL:
    %rep 4
    dw 150, 128, 128, 0
    %endrep

; align 32
RGB2YUV_YMM2_MUL:
    %rep 4
    dw 29, -43, -107, 0
    %endrep

; align 32
RGB2YUV_DELTA:
    %rep 4
    dw 0, 32767, 32767, 0
    %endrep

; align 16
RGB2YUV_INIT_XMM_RGB_SHUFFLE:
    db 0, 1, 2, -1, 3, 4, 5, -1, 6, 7, 8, -1, 9, 10, 11, -1

; align 16
; want xmm1 = [g b r 0 | ...]
RGB2YUV_XMM1_SHUFFLE:
    db 1, 2, 0, 3,  5, 6, 4, 7,  9, 10, 8, 11,  13, 14, 12, 15

; align 16
; want xmm2 = [b r g 0 | ...]
RGB2YUV_XMM2_SHUFFLE:
    db 2, 0, 1, 3,  6, 4, 5, 7,  10, 8, 9, 11,  14, 12, 13, 15

section .text

; MS ABI calling convention
; arg1(rcx) - in         :: const uint8_t*
; arg2(rdx) - out        :: uint8_t* restrict
; arg3(r8) - width      :: size_t
; arg4(r9) - height     :: size_t
; arg5 - in_stride  :: ptrdiff_t
; arg6 - out_stride :: ptrdiff_t
; return - void
rgb2yuv_avx2_improved:
    push r13
    push r14
    push rbx
    ; save ymm registers
    sub rsp, 32 * SIMD_SAVE_REGISTERS
    vmovdqu [rsp + 32 * 0], ymm6
    vmovdqu [rsp + 32 * 1], ymm7
    vmovdqu [rsp + 32 * 2], ymm8
    vmovdqu [rsp + 32 * 3], ymm9

    vmovdqa xmm3, [rel RGB2YUV_INIT_XMM_RGB_SHUFFLE]
    vmovdqa xmm4, [rel RGB2YUV_XMM1_SHUFFLE]
    vmovdqa xmm5, [rel RGB2YUV_XMM2_SHUFFLE]

    vmovdqa ymm6, [rel RGB2YUV_YMM0_MUL]
    vmovdqa ymm7, [rel RGB2YUV_YMM1_MUL]
    vmovdqa ymm8, [rel RGB2YUV_YMM2_MUL]
    vmovdqa ymm9, [rel RGB2YUV_DELTA]

    START_ROW_PROCESSING_RGB2YUV:
    mov rax, r8 ; how many pixels in row left
    mov r10, rcx
    mov r11, rdx

    PROCESS_SIMD_RGB2YUV:
        cmp rax, 6
        jb SMALL_WIDTH_READ_RGB2YUV
        ; !!! here we require rax >= 6, otherwise we may segfault
        ; read block in xmm
        vmovdqu xmm0, [r10]

        RUN_RGB2YUV: ; after this procedure answer is in xmm0
        ; xmm0 = [r g b | r g b | r g b | r g b | r g b | r]
        ; now get rid of useless last pixel
            vpshufb xmm0, xmm0, xmm3
            ; xmm0 = [r g b 0 | r g b 0 | r g b 0 | r g b 0]
            ; now want to have xmm1 and xmm2 with shuffled bytes
            vpshufb xmm1, xmm0, xmm4
            vpshufb xmm2, xmm0, xmm5
            ; all bytes are in correct order
            ; extend them to words
            vpmovzxbw ymm0, xmm0
            vpmovzxbw ymm1, xmm1
            vpmovzxbw ymm2, xmm2
            ; now all are words

            vpmullw ymm0, ymm6
            vpmullw ymm1, ymm7
            vpmullw ymm2, ymm8

            vpaddw ymm0, ymm0, ymm9
            vpaddw ymm1, ymm1, ymm2
            vpaddw ymm0, ymm0, ymm1
            ; multiplied and summarized now
            ; shift
            vpsrlw ymm0, ymm0, 8 ; divide all by 256

            vpackuswb ymm0, ymm0, ymm0 ; pack words to bytes
            ; now ymm0 = [y0 cb0 cr0 0  y1 cb1 cr1 0 | y0 cb0 cr0 0  y1 cb1 cr1 0 | y2 ...]
            vpermq ymm0, ymm0, 0b00001000

            cmp rax, 4
            jb SMALL_WIDTH_WRITE_RGB2YUV
            ; now answer in xmm
            vmovdqu [r11], xmm0 ; stored
            add r10, 3 * 4
            add r11, 4 * 4
            sub rax, 4
            jmp PROCESS_SIMD_RGB2YUV

        FINISHED_ROW_RGB2YUV:
        add rcx, [rsp + (8 + 32 + 32 * SIMD_SAVE_REGISTERS) + 3 * 8] ; add in_stride
        add rdx, [rsp + (8 + 32 + 32 * SIMD_SAVE_REGISTERS) + 4 * 8] ; add out_stride
        dec r9 ; row processed, decrement now
    jnz START_ROW_PROCESSING_RGB2YUV

    ; restore simd registers
    vmovdqu ymm6, [rsp + 32 * 0]
    vmovdqu ymm7, [rsp + 32 * 1]
    vmovdqu ymm8, [rsp + 32 * 2]
    vmovdqu ymm9, [rsp + 32 * 3]
    add rsp, 32 * SIMD_SAVE_REGISTERS

    pop rbx
    pop r14
    pop r13
    ret

    SMALL_WIDTH_READ_RGB2YUV:
        cmp rax, 0
        je FINISHED_ROW_RGB2YUV

        xor rbx, rbx
        xor r14, r14
        sub rsp, 16

        SMALL_COPY_TO_STACK_LOOP_RGB2YUV:
            mov r13w, [r10 + r14]
            mov [rsp + r14], r13w
            mov r13b, [r10 + r14 + 2]
            mov [rsp + r14 + 2], r13b
            inc rbx
            add r14, 3 ; increment read pos
            cmp rbx, rax
            jne SMALL_COPY_TO_STACK_LOOP_RGB2YUV

        vmovdqu xmm0, [rsp]
        add rsp, 16
        jmp RUN_RGB2YUV

    SMALL_WIDTH_WRITE_RGB2YUV:
        sub rsp, 16
        xor rbx, rbx ; pixel index
        vmovdqu [rsp], xmm0

        SMALL_COPY_TO_DST_LOOP_RGB2YUV:
            mov r13d, [rsp + rbx * 4] ; read 4 bytes
            mov [r11], r13d
            inc rbx
            add r11, 4 ; update dst index
            add r10, 3 ; we really read this pixel, so skip it
            cmp rbx, rax
            jne SMALL_COPY_TO_DST_LOOP_RGB2YUV

        xor rax, rax
        add rsp, 16
        jmp PROCESS_SIMD_RGB2YUV
