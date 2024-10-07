bits 64
default rel

segment .data
   fmt_int   DB "%lld", 0
   fmt_float DB "%lf", 0
   fmt_string DB "%s", 0
   fmt_address DB "0x%p", 0
   string_false DB "false", 0
   string_true  DB "true", 0
   string_assert_fail  DB "Assertion failed at line %d", 10, 0
   CF0 DD 1234.1234568
   CF1 DQ 1234.123456789000102
   CS2 DB `%f`, 10, 0 
   CS3 DB `%f`, 10, 0 
   CS4 DB `%lf`, 10, 0 

segment .text
   global main
   extern printf
   extern sprintf
   extern ExitProcess
   extern malloc

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

   ; initialization of 'x'
   mov		DWORD -4[rbp], 0
   mov		rax, 0
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-4[rbp], xmm0

   ; initialization of 'y'
   mov		DWORD -8[rbp], 0
   movss		xmm0, [CF0]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-8[rbp], eax

   ; initialization of 'z'
   mov		DWORD -16[rbp], 0
   movsd		xmm0, [CF1]
   movq		rax, xmm0
   push		rax
   pop		rax
   mov		-16[rbp], rax

   ; expression of print
   mov		eax, -4[rbp]
   push		rax
   pop		rdx
   movq		xmm0, rdx
   cvtss2sd	xmm0, xmm0
   movq		rdx, xmm0
   mov		rcx, CS2
   call		printf

   ; expression of print
   mov		eax, -8[rbp]
   push		rax
   pop		rdx
   movq		xmm0, rdx
   cvtss2sd	xmm0, xmm0
   movq		rdx, xmm0
   mov		rcx, CS3
   call		printf

   ; expression of print
   mov		rax, -16[rbp]
   push		rax
   pop		rdx
   mov		rcx, CS4
   call		printf
L0:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret
