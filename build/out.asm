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
   sub		rsp, 48

   ; initialization of 'x'
   mov		BYTE -1[rbp], 0
   push		1
   pop		rax
   not		rax
   push		rax
   push		0

   pop		rbx
   pop		rax
   and		al, bl
   push		rax


   ; putting result into 'x'
   pop		rax
   mov		BYTE -1[rbp], al

   ; initialization of 'y'
   mov		DWORD -8[rbp], 0
   push		5
   push		3
   pop		rax
   neg		rax
   push		rax
   pop		rax
   neg		rax
   push		rax
   pop		rax
   neg		rax
   push		rax

   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax


   ; putting result into 'y'
   pop		rax
   mov		DWORD -8[rbp], eax

   ; expression of print
   mov		al, BYTE -1[rbp]
   push		rax

   ; call to print
   pop		rax
   cmp		al, 0
   jnz		L1
   mov		rcx, string_false
   jmp		L2
L1:
   mov		rcx, string_true
L2:
   call		printf

   ; expression of print
   mov		eax, DWORD -8[rbp]
   push		rax

   ; call to print
   pop		rdx
   mov		rcx, fmt_int
   call		printf
L0:
   mov		rax, 0
   add		rsp, 48
   pop		rbp
   ret
