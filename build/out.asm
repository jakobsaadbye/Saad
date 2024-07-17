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
   sub		rsp, 64

   ; initialization of 'a'
   mov		DWORD -8[rbp], 0
   mov		DWORD -4[rbp], 0
   push		1
   pop	rax
   cvtsi2ss	xmm0, rax
   movss		DWORD -8[rbp], xmm0
   push		4
   pop	rax
   cvtsi2ss	xmm0, rax
   movss		DWORD -4[rbp], xmm0

   ; initialization of 'b'
   mov		DWORD -16[rbp], 0
   mov		DWORD -12[rbp], 0
   push		3
   pop	rax
   cvtsi2ss	xmm0, rax
   movss		DWORD -16[rbp], xmm0
   push		9
   pop	rax
   cvtsi2ss	xmm0, rax
   movss		DWORD -12[rbp], xmm0

   ; initialization of 'c'
   mov		DWORD -24[rbp], 0
   mov		DWORD -20[rbp], 0
   movss		xmm0, -8[rbp]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, -16[rbp]
   sub		rsp, 4
   movss		[rsp], xmm0

   movss		xmm1, [rsp]
   add		rsp, 4
   movss		xmm0, [rsp]
   add		rsp, 4
   addss		xmm0, xmm1
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, [rsp]
   add		rsp, 4
   movss		DWORD -24[rbp], xmm0
   movss		xmm0, -4[rbp]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, -12[rbp]
   sub		rsp, 4
   movss		[rsp], xmm0

   movss		xmm1, [rsp]
   add		rsp, 4
   movss		xmm0, [rsp]
   add		rsp, 4
   addss		xmm0, xmm1
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, [rsp]
   add		rsp, 4
   movss		DWORD -20[rbp], xmm0

   ; expression of print
   movss		xmm0, -24[rbp]
   sub		rsp, 4
   movss		[rsp], xmm0

   ; call to print
   movss		xmm0, [rsp]
   add		rsp, 4
   cvtss2sd	xmm0, xmm0
   movapd	xmm1, xmm0
   movq		rdx, xmm0
   mov		rcx, fmt_float
   call		printf

   ; expression of print
   movss		xmm0, -20[rbp]
   sub		rsp, 4
   movss		[rsp], xmm0

   ; call to print
   movss		xmm0, [rsp]
   add		rsp, 4
   cvtss2sd	xmm0, xmm0
   movapd	xmm1, xmm0
   movq		rdx, xmm0
   mov		rcx, fmt_float
   call		printf
L0:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret
