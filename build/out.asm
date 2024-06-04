bits 64
default rel

segment .data
   fmt_int   DB "%d", 10, 0
   fmt_float DB "%f", 10, 0
   fmt_string DB "%s", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0

segment .text
   global main
   extern printf
   extern ExitProcess


main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64

   ; initialization of 'a'
   mov		DWORD -4[rbp], 0
   push		0


   ; putting result into 'a'
   pop		rax
   mov		DWORD -4[rbp], eax

   ; initialization of 'b'
   mov		DWORD -8[rbp], 0
   push		10


   ; putting result into 'b'
   pop		rax
   mov		DWORD -8[rbp], eax
   mov		eax, DWORD -4[rbp]
   push		rax
   mov		eax, DWORD -8[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		-12[rbp], eax
   mov		-16[rbp], ebx
   mov		eax, -12[rbp]
   mov		-20[rbp], eax
L1:
   mov		eax, -16[rbp]
   cmp		-20[rbp], eax
   jg		L2

   ; expression of print
   mov		eax, DWORD -20[rbp]
   push		rax

   ; call to print
   pop		rdx
   mov		rcx, fmt_int
   call		printf
   inc		DWORD -20[rbp]
   jmp		L1
L2:
L0:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret
