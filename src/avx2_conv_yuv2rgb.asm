global yuv2rgb_avx2

section .rodata

align 32
YUV2RGB_MUL_YMM0:
    dw 64, -22, 0, 0, 64, -22, 0, 0, 64, -22, 0, 0, 64, -22, 0, 0

align 32
YUV2RGB_MUL_YMM1:
    dw 90, 64, 113, 0, 90, 64, 113, 0, 90, 64, 113, 0, 90, 64, 113, 0

align 32
YUV2RGB_MUL_YMM2:
    dw 0, -46, 64, 0, 0, -46, 64, 0, 0, -46, 64, 0, 0, -46, 64, 0

align 32
YUV2RGB_BIAS:
    %rep 4
    dw -90 * 128, 68 * 128, -113 * 128, 0
    %endrep

align 32
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
; arg5 - in_stride  :: ptrdiff_t
; arg6 - out_stride :: ptrdiff_t
; return - void
yuv2rgb_avx2:

    vpxor ymm3, ymm3, ymm3 ; ymm3 = 0

    START_ROW_PROCESSING:
    mov rax, r8 ; how many pixels in row left
    mov r10, rcx
    mov r11, rdx

    PROCESS_SIMD:
    cmp rax, 6 ; 6 * 3 = 18 > 16, so can just write xmm
    jb SMALL_WIDTH ; too small width, not implemented

    ; ymm0 = [y cb cr x | y cb cr x | y cb cr x | y cb cr x]
    ; ymm1 = [cr y cb x | cr y cb x | cr y cb x | cr y cb x]
    ; ymm2 = [cb cr y x | cb cr y x | cb cr y x | cb cr y x]
    vmovdqu xmm0, [r10] ; read 16 bytes, need to unpack them
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

    movdqu [r11], xmm0 ; stored
    add r10, 4 * 4 ;
    add r11, 3 * 4
    sub rax, 4
    jmp PROCESS_SIMD

    FINISHED_ROW:
    add rcx, [rsp + 8 + 32 + 0 * 8] ; add in_stride
    add rdx, [rsp + 8 + 32 + 1 * 8] ; add out_stride
    dec r9 ; row processed, decrement now
    jnz START_ROW_PROCESSING

    DONE:
    ret

    SMALL_WIDTH:
    cmp rax, 0
    je FINISHED_ROW

    ; save registers for calculations
    ; rax is a register, which used as a loop counter, so save and restore it in loop
    push rcx ; here keep sum
    push rdx
    push r12
    push r13
    push r14
    push rbx

    mov rbx, 255
    xor rdx, rdx

    MANUAL_YUV2RGB_LOOP:
    push rax
    ; ALL MANIPULATIONS ARE SIGNED

    ;mov [r11], WORD 0 ; clear place for rgb pixel
    ;mov [r11 + 2], BYTE 0
    movzx r12, BYTE [r10] ; y
    movzx r13, BYTE [r10 + 1] ; cb
    movzx r14, BYTE [r10 + 2] ; cr

    shl r12, 6 ; r12 = 64 * y

    ; r = 64 * y + 90 * cr + 90 * 128
    mov rcx, r12
    ; rcx = 64 * y

    lea rax, [r14 - 128] ; cr - 128
    imul rax, 90 ; rax = (cr - 128) * 90
    add rcx, rax

    ; shift and saturate r
    sar rcx, 6
    cmp rcx, rbx
    cmovge rcx, rbx ; 255
    cmp rcx, rdx
    cmovle rcx, rdx  ; if < 0, then = 0

    mov [r11], cl ; write r

    ; g = 64 * y - 22 * cb - 46 * cr + 68 * 128
    mov rcx, r12

    lea rax, [r13 - 128] ; cb
    imul rax, 22 ; rax = (cb - 128) * 22
    sub rcx, rax

    lea rax, [r14 - 128] ; cr - 128
    imul rax, 46 ; rax = (cb - 128) * 22
    sub rcx, rax

    ; shift and saturate g
    sar rcx, 6
    cmp rcx, rbx
    cmovge rcx, rbx ; 255
    cmp rcx, rdx
    cmovle rcx, rdx ; if < 0, then = 0

    mov [r11 + 1], cl

    ; b = 64 * y - 22 * cb - 46 * cr + 68 * 128
    mov rcx, r12

    lea rax, [r13 - 128] ; cb
    imul rax, 113 ; rax = (cb - 128) * 113
    add rcx, rax

    ; shift and saturate b
    sar rcx, 6
    cmp rcx, rbx
    cmovge rcx, rbx ; 255
    cmp rcx, rdx
    cmovle rcx, rdx ; if < 0, then = 0

    mov [r11 + 2], cl

    pop rax

    add r10, 4
    add r11, 3
    dec rax
    jnz MANUAL_YUV2RGB_LOOP

    ; restore registers
    pop rbx
    pop r14
    pop r13
    pop r12
    pop rdx
    pop rcx
    jmp FINISHED_ROW
