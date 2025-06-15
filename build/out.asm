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
   C_PI DD 3.141593
   C_TAU DD 6.283186
   CS0 DB `sin(%f)=%lf`, 10, 0 
segment .text
   global main
   extern printf
   extern sprintf
   extern ExitProcess
   extern malloc
   extern sin
   extern cos
   extern floor


assert:
   cmp		cl, 0
   jz 		assert_fail
   ret
assert_fail:
   mov		rcx, string_assert_fail
   call		printf
   mov		rcx, 1
   call		ExitProcess


; bytes total    : 0
; bytes arguments: 0
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 32

   ; expression of print
   movss		xmm0, [C_PI]
   movd		eax, xmm0
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   movss		xmm0, [C_PI]
   movd		eax, xmm0
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   call		sin
   movq		rax, xmm0
   push		rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   call		printf
L0:
   mov		rax, 0
   add		rsp, 32
   pop		rbp
   ret
