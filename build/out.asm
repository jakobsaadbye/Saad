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
   CS0 DB `{ x = %f, y = %f }`, 10, 0 

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

; bytes total    : 88
; bytes arguments: 0
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 120

   ; initialization of 'a'
   mov		QWORD -16[rbp], 0
   mov		QWORD -8[rbp], 0
   mov		DWORD -48[rbp], 0
   mov		DWORD -44[rbp], 0
   mov		DWORD -40[rbp], 0
   mov		DWORD -36[rbp], 0
   mov		DWORD -32[rbp], 0
   mov		DWORD -28[rbp], 0
   mov		DWORD -24[rbp], 0
   mov		DWORD -20[rbp], 0
   mov		rax, 1
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-48[rbp], xmm0
   mov		rax, 2
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-44[rbp], xmm0
   mov		rax, 2
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-40[rbp], xmm0
   mov		rax, 3
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-36[rbp], xmm0
   mov		rax, 3
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-32[rbp], xmm0
   mov		rax, 4
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-28[rbp], xmm0
   mov		rax, 4
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-24[rbp], xmm0
   mov		rax, 5
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-20[rbp], xmm0
   push		4
   lea		rax, -48[rbp]
   push		rax
   pop		rax
   pop		rcx
   mov		QWORD -16[rbp], rax
   mov		QWORD -8[rbp], rcx

   ; initialization of 'b'
   mov		DWORD -56[rbp], 0
   mov		DWORD -52[rbp], 0
   mov		rax, 4
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-56[rbp], xmm0
   mov		rax, 9
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-52[rbp], xmm0
   mov		rax, QWORD -16[rbp]
   mov		rbx, QWORD -8[rbp]
   push		rbx
   push		rax
   ; For-loop
   pop		rax
   pop		rbx
   mov		-72[rbp], rax     ; data
   mov		-80[rbp], rbx     ; count
   mov		QWORD -88[rbp], 0 ; index
L4:
   mov		rbx, -80[rbp]
   mov		rax, -88[rbp]
   cmp		rax, rbx
   jge		L6
   mov		rbx, -72[rbp]
   imul		rax, 8
   add		rbx, rax
   mov		rax, QWORD [rbx]
   mov		-64[rbp], rax 

   ; expression of print
   lea		rax, -64[rbp]
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
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   call		printf
L5:
   inc		QWORD -88[rbp]
   jmp		L4
L6:
L3:
   mov		rax, 0
   add		rsp, 120
   pop		rbp
   ret
