bits 64
default rel

segment .data
   fmt_int   DB "%d", 10, 0
   fmt_float DB "%f", 10, 0
   fmt_string DB "%s", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0
   string_assert_fail  DB "Assertion failed at line %d", 10, 0

segment .text
   global main
   extern printf
   extern ExitProcess

assert:
   cmp		cl, 0
   jz 		assert_fail
   ret
assert_fail:
   mov		rcx, string_assert_fail
   call		printf
   mov		rcx, 1
   call		ExitProcess


main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48

   ; initialization of 'a'
   mov		DWORD -4[rbp], 0
   push		5
   push		4
   pop		rbx
   pop		rax
   imul		rax, rbx
   push		rax
   push		3
   push		2
   pop		rbx
   pop		rax
   imul		rax, rbx
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   push		2
   pop		rbx
   pop		rax
   cqo
   idiv		rbx
   push		rax


   ; putting result into 'a'
   pop		rax
   mov		DWORD -4[rbp], eax

   ; initialization of 'b'
   mov		DWORD -8[rbp], 0
   push		3
   push		2
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax


   ; putting result into 'b'
   pop		rax
   mov		DWORD -8[rbp], eax

   ; expression of print
   mov		eax, DWORD -4[rbp]
   push		rax

   ; call to print
   pop		rdx
   mov		rcx, fmt_int
   call		printf

   ; expression of print
   mov		eax, DWORD -8[rbp]
   push		rax

   ; call to print
   pop		rdx
   mov		rcx, fmt_int
   call		printf
   mov		eax, DWORD -4[rbp]
   push		rax
   push		13

   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 7
   call		assert
   mov		eax, DWORD -8[rbp]
   push		rax
   push		1

   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 8
   call		assert
L0:
   mov		rax, 0
   add		rsp, 48
   pop		rbp
   ret
