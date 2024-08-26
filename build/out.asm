bits 64
default rel

segment .data
   fmt_int   DB "%lld", 10, 0
   fmt_float DB "%lf", 10, 0
   fmt_string DB "%s", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0
   string_assert_fail  DB "Assertion failed at line %d", 10, 0
   C_PI DD 3.141590
   C_TAU DD 6.283180

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

enum_str:
   mov		rcx, fmt_string
   jmp		enum_end
enum_int:
   mov		rcx, fmt_int
   mov		rdx, rax
enum_end:
   pop		rbx
   call		printf
   push		rbx
   ret


main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 32
   movss		xmm0, [C_TAU]
   sub		rsp, 4
   movss		[rsp], xmm0
   mov		rax, 2
   push		rax
   pop		rbx
   cvtsi2ss	xmm1, rbx
   movss		xmm0, [rsp]
   add		rsp, 4
   divss		xmm0, xmm1
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, [C_PI]
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
   mov		rdx, 8
   call		assert
   push		-1
   pop		rcx
   mov		rdx, 9
   call		assert
L0:
   mov		rax, 0
   add		rsp, 32
   pop		rbp
   ret
