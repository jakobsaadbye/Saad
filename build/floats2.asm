bits 64
default rel

segment .data
    fmt_float DB "%lf", 10, 0
    c0 DQ 2.5

segment .text
    global main
    extern ExitProcess
    extern printf


; a = 2.5 + 1
main:
    push    rbp
    mov     rbp, rsp
    sub     rsp, 32

    ; allocate space for 4-byte float
    sub     rsp, 8
    mov     QWORD [rbp - 8], 0

    movq    xmm0, [c0]
    sub     rsp, 8                  ; allocate space for the big ass register
    movq    [rsp], xmm0             ; 'push' xmm0 on the stack

    mov     rax, 1                  
    cvtsi2sd xmm0, rax              ; 'cast' 1 to float
    sub     rsp, 8
    movq    [rsp], xmm0

    movq    xmm0, [rsp]             ; pop rhs
    add     rsp, 8
    movq    xmm1, [rsp]             ; pop lhs
    add     rsp, 8
    addsd   xmm0, xmm1              ; do the +
    sub     rsp, 8                  ; push value          
    movq    [rsp], xmm0

    ; assign the value to 'a'
    movq    xmm0, [rsp]             ; pop the value off the stack
    add     rsp, 8                 
    movlps  QWORD [rbp - 8], xmm0   ; move 8 of the 16 bytes in xmm0 to our 8-byte variable overriding some of the memory

    ; expr inside print
    mov     rax, [rbp - 8]
    push    rax

    pop     rax
    movq    xmm1, rax
    mov     rdx, rax
    mov     rcx, fmt_float
    call    printf

    ; cleanup stack pointer after variable declaration
    add     rsp, 8

    pop     rbp
    mov     rcx, 69
    call    ExitProcess