bits 64
default rel

segment .data
   fmt_int   DB "%lld", 0
   fmt_float DB "%lf", 0
   fmt_string DB "%s", 0
   fmt_address DB "0x%lx", 0
   string_false DB "false", 0
   string_true  DB "true", 0
   string_assert_fail  DB "Assertion failed at line %d", 10, 0
   CF0 DD 3.1415927
segment .text
   global main
   extern ExitProcess
   extern printf
   extern sprintf
   extern malloc
   extern calloc
   extern free
   extern memset
   extern memcpy


assert:
   cmp		cl, 0
   jz 		assert_fail
   ret
assert_fail:
   mov		rcx, string_assert_fail
   call		printf
   mov		rcx, 1
   call		ExitProcess


; bytes locals   : 24
; bytes temp     : 0
; bytes total    : 64
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64

   ; Ln 2: $pi : float = -4
   movss		xmm0, [CF0]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-4[rbp], eax

   ; Ln 3: $tau : float = -8
   mov		eax, -4[rbp]
   push		rax
   mov		rax, 2
   push		rax
   pop		rbx
   cvtsi2ss	xmm1, ebx
   pop		rax
   movd		xmm0, eax
   mulss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-8[rbp], eax

   ; Ln 4: $pi_again : float = -12
   mov		eax, -8[rbp]
   push		rax
   mov		rax, 2
   push		rax
   pop		rbx
   cvtsi2ss	xmm1, ebx
   pop		rax
   movd		xmm0, eax
   divss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-12[rbp], eax
   mov		eax, -12[rbp]
   push		rax
   mov		eax, -4[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   comiss	xmm0, xmm1
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 6
   call		assert
L0:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret
