bits 64
default rel

segment .text
    global main
    extern ExitProcess
    extern printf

main:
    push    rbp
    mov     rbp, rsp
    sub     rsp, 32

    sub     rsp, 8               ; allocate space for a
    mov     QWORD [rbp - 8], 0  ; 0-initialize a

    mov     rax, 6
    push    rax
    mov     rax, 4
    push    rax

    mov     rdx, 0      ; clear
    pop     rbx         ; divisor
    pop     rax         ; divident
    div     rbx
    push    rdx         ; remainder
    push    rax         ; quotient

    pop     rax         ; quotient
    pop     rbx         ; remainder
    mov     [rbp - 8], rbx       

    mov     rax, [rbp - 8]  
    pop     rbp
    mov     rcx, rax
    call    ExitProcess