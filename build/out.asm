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
   CS0 DB `v1 = { x = %f, y = %f, z = %f }`, 10, 0 
   CS1 DB `v2 = { x = %f, y = %f, z = %f }`, 10, 0 
   CS2 DB `v3 = { x = %f, y = %f, z = %f }`, 10, 0 
   CS3 DB `v4 = { x = %f, y = %f, z = %f }`, 10, 0 
   CS4 DB `v5 = { x = %f, y = %f, z = %f }`, 10, 0 
   CS5 DB `v6 = { x = %f, y = %f, z = %f }`, 10, 0 
   CS6 DB `v7 = { x = %f, y = %f, z = %f }`, 10, 0 
   CF7 DD 5.5000000
   CS8 DB `%f`, 10, 0 
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


; bytes locals   : 84
; bytes temp     : 0
; bytes total    : 128
foo:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 128
   ; Copy v1 -> -12
   mov		rax, rcx
   lea		rbx, -12[rbp]
   mov		ecx, 0[rax]
   mov		0[rbx], ecx
   mov		ecx, 4[rax]
   mov		4[rbx], ecx
   mov		ecx, 8[rax]
   mov		8[rbx], ecx
   ; Copy v2 -> -24
   mov		rax, rdx
   lea		rbx, -24[rbp]
   mov		ecx, 0[rax]
   mov		0[rbx], ecx
   mov		ecx, 4[rax]
   mov		4[rbx], ecx
   mov		ecx, 8[rax]
   mov		8[rbx], ecx
   ; Copy v3 -> -36
   mov		rax, r8 
   lea		rbx, -36[rbp]
   mov		ecx, 0[rax]
   mov		0[rbx], ecx
   mov		ecx, 4[rax]
   mov		4[rbx], ecx
   mov		ecx, 8[rax]
   mov		8[rbx], ecx
   ; Copy v4 -> -48
   mov		rax, r9 
   lea		rbx, -48[rbp]
   mov		ecx, 0[rax]
   mov		0[rbx], ecx
   mov		ecx, 4[rax]
   mov		4[rbx], ecx
   mov		ecx, 8[rax]
   mov		8[rbx], ecx
   mov		rax, [rbp+48]
   ; Copy v5 -> -60
   mov		rax, rax
   lea		rbx, -60[rbp]
   mov		ecx, 0[rax]
   mov		0[rbx], ecx
   mov		ecx, 4[rax]
   mov		4[rbx], ecx
   mov		ecx, 8[rax]
   mov		8[rbx], ecx
   mov		rax, [rbp+56]
   ; Copy v6 -> -72
   mov		rax, rax
   lea		rbx, -72[rbp]
   mov		ecx, 0[rax]
   mov		0[rbx], ecx
   mov		ecx, 4[rax]
   mov		4[rbx], ecx
   mov		ecx, 8[rax]
   mov		8[rbx], ecx
   mov		rax, [rbp+64]
   ; Copy v7 -> -84
   mov		rax, rax
   lea		rbx, -84[rbp]
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
   mov		rcx, CS0
   call		printf

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
   mov		rcx, CS1
   call		printf

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

   ; expression of print
   lea		rax, -48[rbp]
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
   mov		rcx, CS3
   call		printf

   ; expression of print
   lea		rax, -60[rbp]
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
   mov		rcx, CS4
   call		printf

   ; expression of print
   lea		rax, -72[rbp]
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
   mov		rcx, CS5
   call		printf

   ; expression of print
   lea		rax, -84[rbp]
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
   mov		rcx, CS6
   call		printf
   movss		xmm0, [CF7]
   movd		eax, xmm0
   push		rax
   jmp		L0
L0:
   pop		rax
   movd		xmm0, eax
   add		rsp, 128
   pop		rbp
   ret

; bytes locals   : 4
; bytes temp     : 180
; bytes total    : 224
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 224

   ; Ln 31: $v = -4
   mov		rax, 1
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-20[rbp], xmm0
   mov		rax, 2
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-16[rbp], xmm0
   mov		rax, 3
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-12[rbp], xmm0
   lea		rax, -20[rbp]
   push		rax
   mov		rax, 4
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-32[rbp], xmm0
   mov		rax, 5
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-28[rbp], xmm0
   mov		rax, 6
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-24[rbp], xmm0
   lea		rax, -32[rbp]
   push		rax
   mov		rax, 7
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-44[rbp], xmm0
   mov		rax, 8
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-40[rbp], xmm0
   mov		rax, 9
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-36[rbp], xmm0
   lea		rax, -44[rbp]
   push		rax
   mov		rax, 10
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-56[rbp], xmm0
   mov		rax, 11
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-52[rbp], xmm0
   mov		rax, 12
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-48[rbp], xmm0
   lea		rax, -56[rbp]
   push		rax
   mov		rax, 13
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-68[rbp], xmm0
   mov		rax, 14
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-64[rbp], xmm0
   mov		rax, 15
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-60[rbp], xmm0
   lea		rax, -68[rbp]
   push		rax
   mov		rax, 16
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-80[rbp], xmm0
   mov		rax, 17
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-76[rbp], xmm0
   mov		rax, 18
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-72[rbp], xmm0
   lea		rax, -80[rbp]
   push		rax
   mov		rax, 19
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-92[rbp], xmm0
   mov		rax, 20
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-88[rbp], xmm0
   mov		rax, 21
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-84[rbp], xmm0
   lea		rax, -92[rbp]
   push		rax
   pop		rax
   mov		-100[rbp], rax
   pop		rax
   mov		-108[rbp], rax
   pop		rax
   mov		-116[rbp], rax
   pop		rax
   mov		r9 , rax
   pop		rax
   mov		r8 , rax
   pop		rax
   mov		rdx, rax
   pop		rax
   mov		rcx, rax
   mov		rax, QWORD -116[rbp]
   mov		32[rsp], rax
   mov		rax, QWORD -108[rbp]
   mov		40[rsp], rax
   mov		rax, QWORD -100[rbp]
   mov		48[rsp], rax
   call		foo
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-4[rbp], eax

   ; expression of print
   mov		eax, -4[rbp]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS8
   call		printf
L1:
   mov		rax, 0
   add		rsp, 224
   pop		rbp
   ret
