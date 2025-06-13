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
   __Fruit.mango DB ".mango", 0
   __Fruit.kiwi DB ".kiwi", 0
   CS0 DB "Saadbye Inc.", 0 
   CS1 DB "Grønhøjgårdsvej 17", 0 
   CS2 DB `{ x = %f, y = %f, z = %f, w = %f }`, 10, 0 

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

get_enum_string_Fruit:
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
   mov		rax, __Fruit.mango
   ret
enum_case_1:
   mov		rax, __Fruit.kiwi
   ret

; bytes total    : 128
; bytes arguments: 40
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 160

   ; initialization of 'b'
   mov		DWORD -80[rbp], 0
   mov		DWORD -76[rbp], 0
   mov		QWORD -72[rbp], 0
   mov		QWORD -64[rbp], 0
   mov		DWORD -56[rbp], 0
   mov		DWORD -52[rbp], 0
   mov		DWORD -48[rbp], 0
   mov		DWORD -44[rbp], 0
   push		0
   pop		rax
   mov		DWORD -80[rbp], eax
   mov		rax, 2
   push		rax
   pop		rax
   mov		DWORD -76[rbp], eax
   mov		rax, CS0
   push		rax
   pop		rax
   mov		QWORD -72[rbp], rax
   mov		rax, CS1
   push		rax
   pop		rax
   mov		QWORD -64[rbp], rax
   mov		rax, 69
   push		rax
   pop		rax
   mov		DWORD -56[rbp], eax
   mov		rax, 420
   push		rax
   pop		rax
   mov		DWORD -52[rbp], eax
   mov		rax, 1337
   push		rax
   pop		rax
   mov		DWORD -48[rbp], eax
   push		1
   pop		rax
   mov		DWORD -44[rbp], eax
   mov		rax, 10
   push		rax
   mov		rax, 20
   push		rax
   pop		rbx
   pop		rax
   mov		-96[rbp], rax
   mov		-104[rbp], rbx
   mov		rax, -96[rbp]
   mov		-88[rbp], rax
   mov		QWORD -112[rbp], 0
L4:
   mov		rax, -104[rbp]
   cmp		-88[rbp], rax
   jg		L6

   ; initialization of 'q'
   mov		DWORD -128[rbp], 0
   mov		DWORD -124[rbp], 0
   mov		DWORD -120[rbp], 0
   mov		DWORD -116[rbp], 0
   mov		rax, QWORD -88[rbp]
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-128[rbp], xmm0
   mov		rax, QWORD -88[rbp]
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-124[rbp], xmm0
   mov		rax, QWORD -88[rbp]
   push		rax
   mov		rax, 2
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-120[rbp], xmm0
   mov		rax, QWORD -88[rbp]
   push		rax
   mov		rax, 3
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-116[rbp], xmm0

   ; expression of print
   lea		rax, QWORD -128[rbp]
   push		rax
   pop		r9
   lea		rbx, 0[r9]
   mov		eax, [rbx]
   push		rax
   pop		rax
   movq		xmm0, rax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   lea		rbx, 4[r9]
   mov		eax, [rbx]
   push		rax
   pop		rax
   movq		xmm0, rax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   lea		rbx, 8[r9]
   mov		eax, [rbx]
   push		rax
   pop		rax
   movq		xmm0, rax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   lea		rbx, 12[r9]
   mov		eax, [rbx]
   push		rax
   pop		rax
   movq		xmm0, rax
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
   mov		rcx, CS2
   mov		rax, QWORD -32[rbp]
   mov		QWORD [rsp + 32], rax
   call		printf
L5:
   inc		QWORD -88[rbp]
   inc		QWORD -112[rbp]
   jmp		L4
L6:
L3:
   mov		rax, 0
   add		rsp, 160
   pop		rbp
   ret
