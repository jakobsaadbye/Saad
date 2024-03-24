bits 64
default rel

segment .data
   fmt_int   DB "%d", 10, 0
   fmt_float DB "%lf", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0
   CS0  DB "Hello World", 0

segment .text
   global main
   extern ExitProcess
   extern printf

; a := "Hello world";
main:
    push	rbp
    mov		rbp, rsp
    sub		rsp, 48

    mov     QWORD [rbp - 8], 0

    mov     rax, CS0
    push    rax

    pop     rax
    mov     QWORD [rbp - 8], rax

    ; call printf
    mov     rcx, [rbp - 8]
    call    printf

    ; cleanup the stack
    pop		rbp
    add		rsp, 48
    mov		rcx, 0
    call		ExitProcess
