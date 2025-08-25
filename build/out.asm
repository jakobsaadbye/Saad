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
   __EntityKind.Monster DB ".Monster", 0
   __EntityKind.Human DB ".Human", 0
   CS0 DB `{ x = %f, y = %f, z = %f, w = %f }`, 10, 0 
   CS1 DB `{ x = %f, y = %f }`, 10, 0 
   CS2 DB `{ x = %f, y = %f, z = %f }`, 10, 0 
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

get_enum_string_EntityKind:
   mov		r8, 0
   cmp		rdx, r8
   jz			enum_case_0
   mov		r8, 1
   cmp		rdx, r8
   jz			enum_case_1
   push		rcx
   mov		r8, rdx
   mov		rdx, fmt_int
   push		r8
   push		rdx
   push		rcx
   call		sprintf
   pop		rax
   pop		rbx
   pop		rbx
   pop		rbx
   ret
enum_case_0:
   mov		rax, __EntityKind.Monster
   ret
enum_case_1:
   mov		rax, __EntityKind.Human
   ret

; bytes locals   : 16
; bytes temp     : 0
; bytes total    : 48
large_return:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48
   mov		-8[rbp], rcx	; Return 0

   ; Ln 32: $r = -32
   mov		rax, 1
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-32[rbp], xmm0
   mov		rax, 4
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-28[rbp], xmm0
   mov		rax, 9
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-24[rbp], xmm0
   lea		rax, -32[rbp]
   push		rax
   jmp		L3
L3:
   pop		rax
   mov		rbx, rax
   mov		rcx, -8[rbp]
   mov		eax, 0[rbx]
   mov		0[rcx], eax
   mov		eax, 4[rbx]
   mov		4[rcx], eax
   mov		eax, 8[rbx]
   mov		8[rcx], eax
   mov		eax, 12[rbx]
   mov		12[rcx], eax
   mov		rax, -8[rbp]
   add		rsp, 48
   pop		rbp
   ret

; bytes locals   : 8
; bytes temp     : 0
; bytes total    : 48
small_return:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48

   ; Ln 37: $r = -8
   mov		rax, 420
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-8[rbp], xmm0
   mov		rax, 69
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-4[rbp], xmm0
   lea		rax, -8[rbp]
   push		rax
   jmp		L4
L4:
   pop		rax
   mov		rax, [rax]
   add		rsp, 48
   pop		rbp
   ret

; bytes locals   : 24
; bytes temp     : 0
; bytes total    : 64
args_and_large_return_value:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64
   mov		-8[rbp], rcx	; Return 0
   movd		-12[rbp], xmm1
   movd		-16[rbp], xmm2
   movd		-20[rbp], xmm3

   ; Ln 42: $r = -36
   mov		eax, -12[rbp]
   push		rax
   pop		rax
   mov		-36[rbp], eax
   mov		eax, -16[rbp]
   push		rax
   pop		rax
   mov		-32[rbp], eax
   mov		eax, -20[rbp]
   push		rax
   pop		rax
   mov		-28[rbp], eax
   lea		rax, -36[rbp]
   push		rax
   jmp		L5
L5:
   pop		rax
   mov		rbx, rax
   mov		rcx, -8[rbp]
   mov		eax, 0[rbx]
   mov		0[rcx], eax
   mov		eax, 4[rbx]
   mov		4[rcx], eax
   mov		eax, 8[rbx]
   mov		8[rcx], eax
   mov		rax, -8[rbp]
   add		rsp, 64
   pop		rbp
   ret

; bytes locals   : 36
; bytes temp     : 16
; bytes total    : 96
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 96

   ; Ln 47: $v1 = -16
   lea		rcx, -56[rbp]		; Return value 0
   call		large_return
   push		rax
   pop		rax
   mov		rbx, rax
   lea		rcx, -16[rbp]
   mov		eax, 0[rbx]
   mov		0[rcx], eax
   mov		eax, 4[rbx]
   mov		4[rcx], eax
   mov		eax, 8[rbx]
   mov		8[rcx], eax
   mov		eax, 12[rbx]
   mov		12[rcx], eax

   ; expression of print
   lea		rax, -16[rbp]
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
   lea		rbx, 12[r9]
   mov		eax, [rbx]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   pop		rax
   mov		QWORD -32[rbp], rax
   pop		rax
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   mov		rax, QWORD -32[rbp]
   mov		QWORD [rsp + 32], rax
   call		printf

   ; Ln 50: $v2 = -24
   call		small_return
   push		rax
   pop		rax
   mov		-24[rbp], rax

   ; expression of print
   lea		rax, -24[rbp]
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
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS1
   call		printf

   ; Ln 53: $v3 = -36
   mov		rax, 27
   push		rax
   mov		rax, 11
   push		rax
   mov		rax, 98
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		eax, xmm0
   movd		xmm3, eax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		eax, xmm0
   movd		xmm2, eax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		eax, xmm0
   movd		xmm1, eax
   lea		rcx, -72[rbp]		; Return value 0
   call		args_and_large_return_value
   push		rax
   pop		rax
   mov		rbx, rax
   lea		rcx, -36[rbp]
   mov		eax, 0[rbx]
   mov		0[rcx], eax
   mov		eax, 4[rbx]
   mov		4[rcx], eax
   mov		eax, 8[rbx]
   mov		8[rcx], eax

   ; expression of print
   lea		rax, -36[rbp]
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
L6:
   mov		rax, 0
   add		rsp, 96
   pop		rbp
   ret
