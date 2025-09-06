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
   CF1 DD 2.0000000
   CF2 DD 3.0000000
   CF3 DD 4.0000000
   CF4 DD 1.0000000
   CF5 DD 2.0000000
   CF6 DD 3.0000000
   CF7 DD 4.0000000
   CF8 DD 1.0000000
   CF9 DD 2.0000000
   CF10 DD 3.0000000
   CF11 DD 4.0000000
   CS12 DB `%f`, 10, 0 
   CS13 DB ``, 10, 0 
   CS14 DB `%f`, 10, 0 
   CS15 DB ``, 10, 0 
   CS16 DB `%f`, 10, 0 
   CS17 DB `cap = %lld`, 10, 0 
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


; bytes locals   : 256
; bytes temp     : 0
; bytes total    : 288
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 288

   ; Ln 11: $static : [10]float = -56
   lea		rax, -40[rbp]
   mov		QWORD -56[rbp], rax
   mov		QWORD -48[rbp], 10
   mov		rcx, rax
   mov		rdx, 0
   mov		r8, 40
   call		memset
   lea		rax, -40[rbp]
   mov		-56[rbp], rax
   mov		QWORD -48[rbp], 10
   movss		xmm0, [CF0]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-40[rbp], eax
   movss		xmm0, [CF1]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-36[rbp], eax
   movss		xmm0, [CF2]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-32[rbp], eax
   movss		xmm0, [CF3]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-28[rbp], eax

   ; Ln 12: $static_inferred : [4]float = -96
   lea		rax, -80[rbp]
   mov		QWORD -96[rbp], rax
   mov		QWORD -88[rbp], 4
   mov		rcx, rax
   mov		rdx, 0
   mov		r8, 16
   call		memset
   lea		rax, -80[rbp]
   mov		-96[rbp], rax
   mov		QWORD -88[rbp], 4
   movss		xmm0, [CF4]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-80[rbp], eax
   movss		xmm0, [CF5]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-76[rbp], eax
   movss		xmm0, [CF6]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-72[rbp], eax
   movss		xmm0, [CF7]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-68[rbp], eax

   ; Ln 13: $dynamic : [..]float = -120
   mov		rdx, 4
   mov		rcx, 4
   call		calloc
   mov		QWORD -120[rbp], rax
   mov		QWORD -112[rbp], 4
   mov		QWORD -104[rbp], 4
   movss		xmm0, [CF8]
   movd		eax, xmm0
   push		rax
   mov		rax, -120[rbp] ; elem 0
   lea		rbx, [rax + 0]
   pop		rax
   mov		[rbx], eax
   movss		xmm0, [CF9]
   movd		eax, xmm0
   push		rax
   mov		rax, -120[rbp] ; elem 1
   lea		rbx, [rax + 4]
   pop		rax
   mov		[rbx], eax
   movss		xmm0, [CF10]
   movd		eax, xmm0
   push		rax
   mov		rax, -120[rbp] ; elem 2
   lea		rbx, [rax + 8]
   pop		rax
   mov		[rbx], eax
   movss		xmm0, [CF11]
   movd		eax, xmm0
   push		rax
   mov		rax, -120[rbp] ; elem 3
   lea		rbx, [rax + 12]
   pop		rax
   mov		[rbx], eax
   lea		rax, -56[rbp]
   push		rax
   ; For-loop
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-136[rbp], rbx     ; data
   mov		-144[rbp], rcx     ; count
   mov		QWORD -152[rbp], 0 ; index
L1:
   mov		rbx, -144[rbp]
   mov		rax, -152[rbp]
   cmp		rax, rbx
   jge		L3
   mov		rbx, -136[rbp]
   imul		rax, 4
   add		rbx, rax
   mov		eax, DWORD [rbx]
   mov		-128[rbp], eax 

   ; expression of print
   mov		eax, -128[rbp]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS12
   call		printf
L2:
   inc		QWORD -152[rbp]
   jmp		L1
L3:

   ; expression of print
   mov		rcx, CS13
   call		printf
   lea		rax, -96[rbp]
   push		rax
   ; For-loop
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-168[rbp], rbx     ; data
   mov		-176[rbp], rcx     ; count
   mov		QWORD -184[rbp], 0 ; index
L4:
   mov		rbx, -176[rbp]
   mov		rax, -184[rbp]
   cmp		rax, rbx
   jge		L6
   mov		rbx, -168[rbp]
   imul		rax, 4
   add		rbx, rax
   mov		eax, DWORD [rbx]
   mov		-160[rbp], eax 

   ; expression of print
   mov		eax, -160[rbp]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS14
   call		printf
L5:
   inc		QWORD -184[rbp]
   jmp		L4
L6:

   ; expression of print
   mov		rcx, CS15
   call		printf
   lea		rax, -120[rbp]
   push		rax
   ; For-loop
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-200[rbp], rbx     ; data
   mov		-208[rbp], rcx     ; count
   mov		QWORD -216[rbp], 0 ; index
L7:
   mov		rbx, -208[rbp]
   mov		rax, -216[rbp]
   cmp		rax, rbx
   jge		L9
   mov		rbx, -200[rbp]
   imul		rax, 4
   add		rbx, rax
   mov		eax, DWORD [rbx]
   mov		-192[rbp], eax 

   ; expression of print
   mov		eax, -192[rbp]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS16
   call		printf
L8:
   inc		QWORD -216[rbp]
   jmp		L7
L9:

   ; expression of print
   mov		rax, QWORD -88[rbp]
   push		rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS17
   call		printf
L0:
   mov		rax, 0
   add		rsp, 288
   pop		rbp
   ret
