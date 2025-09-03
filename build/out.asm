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
   CF0 DD 1.0000000
   CF1 DD 4.0000000
   CS2 DB `{ x = %f, y = %f, z = %f }`, 10, 0 
   CS3	DB "untyped(int)", 10, 0
   CS4 DB `%s`, 10, 0 
segment .text
   global main
   extern ExitProcess
   extern printf
   extern sprintf
   extern malloc
   extern calloc
   extern free
   extern memset


assert:
   cmp		cl, 0
   jz 		assert_fail
   ret
assert_fail:
   mov		rcx, string_assert_fail
   call		printf
   mov		rcx, 1
   call		ExitProcess


; bytes locals   : 12
; bytes temp     : 12
; bytes total    : 64
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64

   ; Ln 12: $v : Vector3 = -12
   movss		xmm0, [CF0]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-28[rbp], eax
   movss		xmm0, [CF1]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-24[rbp], eax
   push		10
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-20[rbp], xmm0
   lea		rax, -28[rbp]
   push		rax
   pop		rax
   lea		rbx, -12[rbp]
   mov		ecx, 0[rax]
   mov		0[rbx], ecx
   mov		ecx, 4[rax]
   mov		4[rbx], ecx
   mov		ecx, 8[rax]
   mov		8[rbx], ecx

   ; expression of print
   lea		rax, -12[rbp]
   push		rax
   pop		r9
   lea		rbx, 0[r9]
   mov		eax, [rbx]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   lea		rbx, 4[r9]
   mov		eax, [rbx]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   lea		rbx, 8[r9]
   mov		eax, [rbx]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   pop		rax
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS2
   call		printf

   ; expression of print
   mov		rax, CS3
   push		rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS4
   call		printf
L0:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret
