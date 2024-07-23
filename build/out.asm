bits 64
default rel

segment .data
   fmt_int   DB "%d", 10, 0
   fmt_float DB "%f", 10, 0
   fmt_string DB "%s", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0
   string_assert_fail  DB "Assertion failed at line %d", 10, 0
   CF0 DD 20.000000
   CF1 DD 0.500000
   CF2 DD 80.000000
   CF3 DD 20.000000

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
   sub		rsp, 64

   ; initialization of 'samoid'
   mov		DWORD -16[rbp], 0
   mov		DWORD -12[rbp], 0
   mov		DWORD -8[rbp], 0
   mov		BYTE -4[rbp], 0
   mov		BYTE -3[rbp], 0
   push		4
   pop		rax
   mov		DWORD -16[rbp], eax
   movss		xmm0, [CF0]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, [rsp]
   add		rsp, 4
   movss		DWORD -12[rbp], xmm0
   movss		xmm0, [CF1]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, [rsp]
   add		rsp, 4
   movss		DWORD -8[rbp], xmm0
   push		-1
   pop		rax
   mov		BYTE -4[rbp], al

   ; initialization of 'bmi'
   mov		DWORD -20[rbp], 0
   movss		xmm0, -12[rbp]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, -8[rbp]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, -8[rbp]
   sub		rsp, 4
   movss		[rsp], xmm0

   movss		xmm1, [rsp]
   add		rsp, 4
   movss		xmm0, [rsp]
   add		rsp, 4
   mulss		xmm0, xmm1
   sub		rsp, 4
   movss		[rsp], xmm0

   movss		xmm1, [rsp]
   add		rsp, 4
   movss		xmm0, [rsp]
   add		rsp, 4
   divss		xmm0, xmm1
   sub		rsp, 4
   movss		[rsp], xmm0


   ; putting result into 'bmi'
   movss		xmm0, [rsp]
   add		rsp, 4
   movss		DWORD -20[rbp], xmm0
   movss		xmm0, -20[rbp]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, [CF2]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm1, [rsp]
   add		rsp, 4
   movss		xmm0, [rsp]
   add		rsp, 4
   comiss		xmm0, xmm1
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 23
   call		assert
   mov		al, BYTE -4[rbp]
   push		rax
   pop		rcx
   mov		rdx, 24
   call		assert
   movss		xmm0, -12[rbp]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, [CF3]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm1, [rsp]
   add		rsp, 4
   movss		xmm0, [rsp]
   add		rsp, 4
   comiss		xmm0, xmm1
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 25
   call		assert
L0:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret
