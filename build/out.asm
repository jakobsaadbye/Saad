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


main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48

   ; initialization of 'a'
   mov		DWORD -4[rbp], 0
   push		4


   ; putting result into 'a'
   pop		rax
   mov		DWORD -4[rbp], eax

   ; initialization of 'b'
   mov		DWORD -8[rbp], 0
   push		2


   ; putting result into 'b'
   pop		rax
   mov		DWORD -8[rbp], eax
   mov		eax, DWORD -8[rbp]
   push		rax
   pop		rax
   mov		edx, eax
   mov		eax, DWORD -4[rbp]
   push		rax
   push		3

   pop		rbx
   pop		rax
   imul		rax, rbx
   push		rax
   pop		rax
   mov		ecx, eax
   call		foo
   add		rsp, 48
   pop		rbp
   ret

foo:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48
   mov		DWORD 16[rbp], ecx
   mov		DWORD 24[rbp], edx

   ; initialization of 'c'
   mov		DWORD -12[rbp], 0
   mov		eax, DWORD 16[rbp]
   push		rax
   mov		eax, DWORD 24[rbp]
   push		rax

   pop		rbx
   pop		rax
   imul		rax, rbx
   push		rax


   ; putting result into 'c'
   pop		rax
   mov		DWORD -12[rbp], eax

   ; expression of print
   mov		eax, DWORD -12[rbp]
   push		rax

   ; call to print
   pop		rdx
   mov		rcx, fmt_int
   call		printf
   add		rsp, 48
   pop		rbp
   ret
