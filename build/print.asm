bits 64
default rel

segment .data
    fmt_int DB "%d", 0

segment .text
   global main
   extern ExitProcess
   extern printf

main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 32

   ; initialization of 'a'
   sub		rsp, 8
   mov		QWORD [rbp - 8], 0

   mov		rax, 5
   push		rax
   mov		rax, 3
   push		rax

   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax

   ; putting result into 'a'
   pop		rax
   mov		QWORD [rbp - 8], rax
   mov		rax, [rbp - 8]
   push		rax

   ; call to print
   pop      rdx
   lea      rcx, [fmt_int]
   call     printf

   ; cleanup the stack
   pop		rbp
   add		rsp, 8
   mov		rcx, 0
   call		ExitProcess
