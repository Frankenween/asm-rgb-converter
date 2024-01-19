global rgb2yuv_avx2

section .rodata

align 16
INIT_XMM_RGB_SHUFFLE:
    db 0, 1, 2, -1, 3, 4, 5, -1, 6, 7, 8, -1, 9, 10, 11, -1

align 16
; want xmm1 = [g b r 0 | ...]
XMM1_SHUFFLE:
    db 1, 2, 0, 3,  5, 6, 4, 7,  9, 10, 8, 11,  13, 14, 12, 15

align 16
; want xmm2 = [b r g 0 | ...]
XMM2_SHUFFLE:
    db 2, 0, 1, 3,  6, 4, 5, 7,  10, 8, 9, 11,  14, 12, 13, 15

align 32
YMM0_MUL:
    %rep 4
    dw 77, -85, -21, 0
    %endrep

align 32
YMM1_MUL:
    %rep 4
    dw 150, 128, 128, 0
    %endrep

align 32
YMM2_MUL:
    %rep 4
    dw 29, -43, -107, 0
    %endrep

align 32
RGB2YUV_DELTA:
    %rep 4
    dw 0, 32767, 32767, 0
    %endrep

section .text

; MS ABI calling convention
; arg1(rcx) - in         :: const uint8_t*
; arg2(rdx) - out        :: uint8_t* restrict
; arg3(r8) - width      :: size_t
; arg4(r9) - height     :: size_t
; arg5 - in_stride  :: ptrdiff_t
; arg6 - out_stride :: ptrdiff_t
; return - void
rgb2yuv_avx2:
    push r12

    START_ROW_PROCESSING:
    mov rax, r8 ; how many pixels in row left
    mov r10, rcx
    mov r11, rdx

    PROCESS_SIMD:
    cmp rax, 6
    jb SMALL_WIDTH
    ; !!! here we require rax >= 6, otherwise we may segfault

    ; read block in xmm
    vmovdqu xmm0, [r10]
    ; xmm0 = [r g b | r g b | r g b | r g b | r g b | r]
    ; now get rid of useless last pixel
    vpshufb xmm0, xmm0, [rel INIT_XMM_RGB_SHUFFLE]
    ; xmm0 = [r g b 0 | r g b 0 | r g b 0 | r g b 0]
    ; now want to have xmm1 and xmm2 with shuffled bytes
    vpshufb xmm1, xmm0, [rel XMM1_SHUFFLE]
    vpshufb xmm2, xmm0, [rel XMM2_SHUFFLE]
    ; all bytes are in correct order
    ; extend them to words
    vpmovzxbw ymm0, xmm0
    vpmovzxbw ymm1, xmm1
    vpmovzxbw ymm2, xmm2
    ; now all are words

    vpmullw ymm0, [rel YMM0_MUL]
    vpmullw ymm1, [rel YMM1_MUL]
    vpmullw ymm2, [rel YMM2_MUL]

    vpaddw ymm0, ymm0, [rel RGB2YUV_DELTA]
    vpaddw ymm1, ymm1, ymm2
    vpaddw ymm0, ymm0, ymm1
    ; multiplied and summarized now
    ; shift
    vpsrlw ymm0, ymm0, 8 ; divide all by 256

    vpackuswb ymm0, ymm0, ymm0 ; pack words to bytes
    ; now ymm0 = [y0 cb0 cr0 0  y1 cb1 cr1 0 | y0 cb0 cr0 0  y1 cb1 cr1 0 | y2 ...]
    vpermq ymm0, ymm0, 0b00001000
    ; now answer in xmm
    movdqu [r11], xmm0 ; stored
    add r10, 3 * 4
    add r11, 4 * 4
    sub rax, 4
    jmp PROCESS_SIMD

    FINISHED_ROW:
    add rcx, [rsp + 8 + 32 + 1 * 8] ; add in_stride
    add rdx, [rsp + 8 + 32 + 2 * 8] ; add out_stride
    dec r9 ; row processed, decrement now
    jnz START_ROW_PROCESSING

    DONE:
    pop r12
    ret

    SMALL_WIDTH:
    ; here we do all stuff manually, finish row and go to FINISHED_ROW
    cmp rax, 0
    je FINISHED_ROW
    MANUAL_LOOP:
    push rax

    mov [r11], DWORD 0 ; clear place for yuv pixel

; y = 77 * r + 150 * g + 29 * b
    movzx rax, BYTE [r10] ; r
    imul rax, 77
    mov r12, rax ; r12 = 77 * r

    movzx rax, BYTE [r10 + 1] ; g
    imul rax, 150
    add r12, rax ; r12 = 77 * r + 150 * g

    movzx rax, BYTE [r10 + 2] ; b
    imul rax, 29
    add r12, rax ; r12 = 77 * r + 150 * g + 29 * b

    shr r12, 8
    mov [r11], r12b
; cb
    mov r12, 32767
    movzx rax, BYTE [r10] ; r
    imul rax, 43
    sub r12, rax ; r12 = 32767 - 43 * r

    movzx rax, BYTE [r10 + 1] ; g
    imul rax, 85
    sub r12, rax ; r12 = 32767 - 43 * r - 85 * g

    movzx rax, BYTE [r10 + 2] ; b
    shl rax, 7
    add r12, rax ; r12 = 32767 - 43 * r - 85 * g + 128 * b

    shr r12, 8
    mov [r11 + 1], r12b
; cr
    mov r12, 32767
    movzx rax, BYTE [r10] ; r
    shl rax, 7
    add r12, rax ; r12 = 32767 + 128 * r

    movzx rax, BYTE [r10 + 1] ; g
    imul rax, 107
    sub r12, rax ; r12 = 32767 + 128 * r - 107 * g

    movzx rax, BYTE [r10 + 2] ; b
    imul rax, 21
    sub r12, rax ; r12 = 32767 + 128 * r - 107 * g - 21 * b

    shr r12, 8
    mov [r11 + 2], r12b

    pop rax

    add r10, 3
    add r11, 4
    dec rax
    jnz MANUAL_LOOP
    jmp FINISHED_ROW
