bits 64
default rel

segment .data
   fmt_int   DB "%d", 10, 0
   fmt_float DB "%f", 10, 0
   fmt_string DB "%s", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0
   CS0 DB "Hello world!", 0 

segment .text
   global main
   extern ExitProcess
   extern printf

main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48

   ; initialization of 'a'
   mov		rax, CS0
   push		rax


   ; putting result into 'a'

   ; expression of print
   mov		rax, [rbp - 0]
   push		rax

   ; call to print
   pop		rdx
   mov		rcx, fmt_string
   call		printf

   ; cleanup the stack
   pop		rbp
   add		rsp, 48
   mov		rcx, 0
   call		ExitProcess
