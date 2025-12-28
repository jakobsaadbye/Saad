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
   CF0 DD 7.0000000
   CF1 DD 8.0000000
   CF2 DD 9.0000000
   CS3 DB `%d %d %d %d Vector3{ x = %f, y = %f, z = %f }`, 10, 0 
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


; bytes locals   : 136
; bytes temp     : 0
; bytes total    : 176
add:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 176
   mov		-8[rbp], rcx	; Return (4, Vector3)
   mov		24[rbp], rdx 	; Save arg 0 
   mov		32[rbp], r8 	; Save arg 1 
   mov		40[rbp], r9 	; Save arg 2 
   ; Copy a -> -12
   mov		rax, 24[rbp]
   mov		-12[rbp], eax
   ; Copy b -> -16
   mov		rax, 32[rbp]
   mov		-16[rbp], eax
   ; Copy c -> -20
   mov		rax, 40[rbp]
   mov		-20[rbp], eax
   ; Copy d -> -24
   mov		rax, 48[rbp]
   mov		-24[rbp], eax
   ; Copy e -> -28
   mov		rax, 56[rbp]
   mov		-28[rbp], eax
   ; Copy f -> -32
   mov		rax, 64[rbp]
   mov		-32[rbp], eax
   ; Copy g -> -48
   mov		rax, 72[rbp]
   lea		rbx, -48[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 12
   call		memcpy
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -16[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -20[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -24[rbp]
   movsx		rax, eax
   push		rax
   lea		rax, -48[rbp]
   push		rax
   jmp		L0
L0:
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 12
   call		memcpy
   mov		r9, -8[rbp]
   pop		r8
   pop		rdx
   pop		rcx
   pop		rax
   add		rsp, 176
   pop		rbp
   ret

; bytes locals   : 48
; bytes temp     : 48
; bytes total    : 128
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 128
   ; Ln 18: $a : int = -4[rbp]
   mov		rax, 1
   push		rax
   mov		rax, 2
   push		rax
   mov		rax, 3
   push		rax
   mov		rax, 4
   push		rax
   mov		rax, 5
   push		rax
   mov		rax, 6
   push		rax
   lea		rcx, -60[rbp]
   mov		rdx, 0
   mov		r8, 12
   call		memset
   movss		xmm0, [CF0]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-60[rbp], eax
   movss		xmm0, [CF1]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-56[rbp], eax
   movss		xmm0, [CF2]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-52[rbp], eax
   lea		rax, -60[rbp]
   push		rax
   pop		rax
   mov		-68[rbp], rax
   pop		rax
   mov		-76[rbp], rax
   pop		rax
   mov		-84[rbp], rax
   pop		rax
   mov		-92[rbp], rax
   pop		rax
   mov		r9d, eax
   pop		rax
   mov		r8d, eax
   pop		rax
   mov		edx, eax
   mov		rax, QWORD -92[rbp]
   mov		32[rsp], eax
   mov		rax, QWORD -84[rbp]
   mov		40[rsp], eax
   mov		rax, QWORD -76[rbp]
   mov		48[rsp], eax
   mov		rax, QWORD -68[rbp]
   mov		56[rsp], rax
   lea		rcx, -72[rbp]		; Return value (4, Vector3)
   call		add
   push		r9
   push		r8
   push		rdx
   push		rcx
   push		rax
   pop		rax
   mov		DWORD -4[rbp], eax
   ; Ln 18: $b : int = -8[rbp]
   pop		rax
   mov		DWORD -8[rbp], eax
   ; Ln 18: $c : int = -12[rbp]
   pop		rax
   mov		DWORD -12[rbp], eax
   ; Ln 18: $d : int = -16[rbp]
   pop		rax
   mov		DWORD -16[rbp], eax
   ; Ln 18: $e : Vector3 = -32[rbp]
   pop		rax
   lea		rbx, -32[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 12
   call		memcpy
   ; Ln 21 Print
   mov		eax, DWORD -4[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -8[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -16[rbp]
   movsx		rax, eax
   push		rax
   lea		rax, -32[rbp]
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
   ; Pop print arguments
   pop		rax
   mov		QWORD -56[rbp], rax
   pop		rax
   mov		QWORD -64[rbp], rax
   pop		rax
   mov		QWORD -72[rbp], rax
   pop		rax
   mov		QWORD -80[rbp], rax
   pop		rax
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS3
   mov		rax, -80[rbp]
   mov		QWORD 32[rsp], rax
   mov		rax, -72[rbp]
   mov		QWORD 40[rsp], rax
   mov		rax, -64[rbp]
   mov		QWORD 48[rsp], rax
   mov		rax, -56[rbp]
   mov		QWORD 56[rsp], rax
   call		printf
L1:
   mov		rax, 0
   add		rsp, 128
   pop		rbp
   ret
