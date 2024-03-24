bits 64
default rel

segment .data
    fmt_int   DB "%d", 0

segment .text
    global main
    extern ExitProcess
    extern printf

; a := (1 < 3) && (2 >= 2)
; if a {
;   print(a);
; } else {
;   print(69);
; }
main:
    push    rbp
    mov     rbp, rsp
    sub     rsp, 32

    sub     rsp, 1      ; allocate space for a
    mov     BYTE [rbp - 1], 0

    mov     rax, 1      
    push    rax
    mov     rax, 3
    push    rax

    pop     rbx
    pop     rax
    cmp     rax, rbx
    jl      L1          
    push    0           
    jmp     L2

L1: push    1
L2: 
    mov     rax, 2      
    push    rax
    mov     rax, 2
    push    rax

    pop     rbx
    pop     rax
    cmp     rax, rbx
    jge     L3          
    push    0           
    jmp     L4

L3: push    1
L4:
    pop     rbx         ; carry out the &&
    pop     rax
    and     rax, rbx
    push    rax

    pop     rax         ; assign result to a
    mov     [rbp - 1], rax
    
    mov     rcx, fmt_int ; print result
    mov     rdx, [rbp - 1]
    call    printf
    add     rsp, 8      ; realign the stack

    add     rsp, 1      ; clean up the space for a

    pop     rbp
    mov     rcx, [rbp - 1]
    call    ExitProcess