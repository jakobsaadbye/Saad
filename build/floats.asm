bits 64
default rel

segment .data
    fmt_float DB "%lf", 10, 0
    c0 DQ 2.5
    c1 DQ 3.0

segment .text
    global main
    extern ExitProcess
    extern printf


; a = 2.5 + 3
; print a
main:
    push    rbp
    mov     rbp, rsp
    sub     rsp, 32

    ; allocate space for 4-byte float
    sub     rsp, 4
    mov     QWORD [rbp - 4], 0

    movq    xmm0, [c0]
    sub     rsp, 4                  ; allocate space for the big ass register
    movq    [rsp], xmm0             ; 'push' xmm0 on the stack

    movq    xmm0, [c1]
    sub     rsp, 4              
    movq    [rsp], xmm0

    movq    xmm0, [rsp]             ; pop rhs
    add     rsp, 4
    movq    xmm1, [rsp]             ; pop lhs
    add     rsp, 4
    addsd   xmm0, xmm1              ; do the +
    sub     rsp, 4                  ; push value          
    movq    [rsp], xmm0

    ; assign the value to 'a'
    movq    xmm0, [rsp]             ; pop the value off the stack
    add     rsp, 4                 
    movlps  QWORD [rbp - 4], xmm0   ; move 8 of the 16 bytes in xmm0 to our 4-byte variable overriding some of the memory

    ; expr inside print
    mov     rax, [rbp - 4]
    push    rax

    pop     rax
    movq    xmm1, rax
    mov     rdx, rax
    mov     rcx, fmt_float
    call    printf

    ; cleanup stack pointer after variable declaration
    add     rsp, 4

    pop     rbp
    mov     rcx, 69
    call    ExitProcess