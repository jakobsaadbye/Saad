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
   enum_buffer_0 times 20 DB 0
   enum_buffer_1 times 20 DB 0
   CS2 DB `{ preferred_fruit = %s, blades = %d, manufacturer = { name = %s, address = %s, id = %d, id1 = %d, id2 = %d, fruit_of_the_month = %s } }`, 10, 0 

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

; bytes total    : 132
; bytes arguments: 72
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 168

   ; initialization of 'b'
   mov		DWORD -120[rbp], 0
   mov		DWORD -116[rbp], 0
   mov		QWORD -112[rbp], 0
   mov		QWORD -104[rbp], 0
   mov		DWORD -96[rbp], 0
   mov		DWORD -92[rbp], 0
   mov		DWORD -88[rbp], 0
   mov		DWORD -84[rbp], 0
   push		0
   pop		rax
   mov		DWORD -120[rbp], eax
   mov		rax, 2
   push		rax
   pop		rax
   mov		DWORD -116[rbp], eax
   mov		rax, CS0
   push		rax
   pop		rax
   mov		QWORD -112[rbp], rax
   mov		rax, CS1
   push		rax
   pop		rax
   mov		QWORD -104[rbp], rax
   mov		rax, 69
   push		rax
   pop		rax
   mov		DWORD -96[rbp], eax
   mov		rax, 420
   push		rax
   pop		rax
   mov		DWORD -92[rbp], eax
   mov		rax, 1337
   push		rax
   pop		rax
   mov		DWORD -88[rbp], eax
   push		1
   pop		rax
   mov		DWORD -84[rbp], eax

   ; initialization of 'v'
   mov		DWORD -132[rbp], 0
   mov		DWORD -128[rbp], 0
   mov		DWORD -124[rbp], 0
   mov		rax, 4
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-132[rbp], xmm0
   mov		rax, 9
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-128[rbp], xmm0
   mov		rax, 11
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-124[rbp], xmm0

   ; initialization of 'a'
   mov		QWORD -144[rbp], 0
   lea		rax, -120[rbp]
   push		rax
   pop		rax
   mov		QWORD -144[rbp], rax

   ; expression of print
   mov		rax, -144[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		r9
   lea		rbx, 0[r9]
   mov		eax, DWORD [rbx]
   push		rax
   pop		rdx
   mov		rcx, enum_buffer_0
   call		get_enum_string_Fruit
   push		rax
   lea		rbx, 4[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 8[r9]
   mov		rax, QWORD [rbx]
   push		rax
   lea		rbx, 16[r9]
   mov		rax, QWORD [rbx]
   push		rax
   lea		rbx, 24[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 28[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 32[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 36[r9]
   mov		eax, DWORD [rbx]
   push		rax
   pop		rdx
   mov		rcx, enum_buffer_1
   call		get_enum_string_Fruit
   push		rax
   pop		rax
   mov		QWORD -32[rbp], rax
   pop		rax
   mov		QWORD -40[rbp], rax
   pop		rax
   mov		QWORD -48[rbp], rax
   pop		rax
   mov		QWORD -56[rbp], rax
   pop		rax
   mov		QWORD -64[rbp], rax
   pop		rax
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS2
   mov		rax, QWORD -32[rbp]
   mov		QWORD [rsp + 64], rax
   mov		rax, QWORD -40[rbp]
   mov		QWORD [rsp + 56], rax
   mov		rax, QWORD -48[rbp]
   mov		QWORD [rsp + 48], rax
   mov		rax, QWORD -56[rbp]
   mov		QWORD [rsp + 40], rax
   mov		rax, QWORD -64[rbp]
   mov		QWORD [rsp + 32], rax
   call		printf
L3:
   mov		rax, 0
   add		rsp, 168
   pop		rbp
   ret
