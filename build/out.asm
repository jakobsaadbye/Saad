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
   CF0 DD 100.0000000
   CF1 DD 50.0000000
   CF2 DD 75.0000000
   CF3 DD 100.0000000
   CF4 DD 200.0000000
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


; bytes locals   : 16
; bytes temp     : 0
; bytes total    : 48
foo:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48
   ; Param r
   mov		-8[rbp], rcx
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		eax, [rbx]
   push		rax
   mov		rax, 50
   push		rax
   pop		rbx
   cvtsi2ss	xmm1, ebx
   pop		rax
   movd		xmm0, eax
   comiss	xmm0, xmm1
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L1
   ; block of if
   ; Return
   jmp		L0
   jmp L1
; done
L1:
   ; Ln 12: Assignment
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		eax, [rbx]
   push		rax
   movss		xmm0, [CF0]
   movd		eax, xmm0
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   addss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		[rbx], eax
L0:
   mov		rax, 0
   add		rsp, 48
   pop		rbp
   ret

; bytes locals   : 16
; bytes temp     : 24
; bytes total    : 80
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 80
   ; Ln 16: $r : Rectangle = -16[rbp]
   lea		rcx, -16[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   movss		xmm0, [CF1]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-16[rbp], eax
   movss		xmm0, [CF2]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-12[rbp], eax
   movss		xmm0, [CF3]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-8[rbp], eax
   movss		xmm0, [CF4]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-4[rbp], eax
   lea		rax, -16[rbp]
   push		rax
   pop		rcx
   call		foo
L2:
   mov		rax, 0
   add		rsp, 80
   pop		rbp
   ret
