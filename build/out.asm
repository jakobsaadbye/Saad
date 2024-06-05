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
   extern printf
   extern ExitProcess


main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48

   ; initialization of 'a'
   mov		QWORD -8[rbp], 0
   mov		rax, CS0
   push		rax


   ; putting result into 'a'
   pop		rax
   mov		QWORD -8[rbp], rax

   ; expression of print
   mov		rax, QWORD -8[rbp]
   push		rax

   ; call to print
   pop		rdx
   mov		rcx, fmt_string
   call		printf
L0:
   mov		rax, 0
   add		rsp, 48
   pop		rbp
   ret
