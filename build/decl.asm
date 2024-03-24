bits 64
default rel

segment .text
    global main
    extern ExitProcess
    extern printf

; a = 5 * 3;
; b = 2 + a;


main:
    push    rbp
    mov     rbp, rsp
    sub     rsp, 32

    sub     rsp, 8              ; allocate space for a
    mov     QWORD [rbp - 8], 0  ; 0-initialize a

    mov     rax, 5
    push    rax
    mov     rax, 3
    push    rax

    pop     rax
    pop     rbx
    imul    rax, rbx
    push    rax

    pop     rax                  
    mov     [rbp - 8], rax       ; assign 5*3 to a

    sub     rsp, 8               ; allocate space for b
    mov     QWORD [rbp - 16], 0  ; 0-initialize b

    mov     rax, 2
    push    rax
    mov     rax, [rbp - 8]       ; [rbp - 8] refers to value of 'a'
    push    rax

    pop     rax
    pop     rbx
    add     rax, rbx
    push    rax

    pop     rax                  
    mov     [rbp - 16], rax       ; assign 2+a to b

    mov     rax, [rbp - 16]       ; exit with result of b
    pop     rbp
    mov     rcx, rax
    call    ExitProcess