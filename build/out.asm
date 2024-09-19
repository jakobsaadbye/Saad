bits 64
default rel

segment .data
   fmt_int   DB "%lld", 10, 0
   fmt_float DB "%lf", 10, 0
   fmt_string DB "%s", 10, 0
   fmt_address DB "0x%p", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0
   string_assert_fail  DB "Assertion failed at line %d", 10, 0

segment .text
   global main
   extern printf
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
   sub		rsp, 144

   ; initialization of 'c0'
   mov		DWORD -4[rbp], 0
   mov		rax, 1
   push		rax
   pop		rax
   mov		DWORD -4[rbp], eax

   ; initialization of 'c1'
   mov		DWORD -8[rbp], 0
   mov		rax, 2
   push		rax
   pop		rax
   mov		DWORD -8[rbp], eax

   ; initialization of 'c2'
   mov		DWORD -12[rbp], 0
   mov		rax, 3
   push		rax
   pop		rax
   mov		DWORD -12[rbp], eax

   ; initialization of 'a'
   mov		QWORD -32[rbp], 0
   mov		QWORD -24[rbp], 0
   mov		QWORD -56[rbp], 0
   mov		QWORD -48[rbp], 0
   mov		QWORD -40[rbp], 0
   lea		rax, -4[rbp]
   push		rax
   pop		rax
   mov		QWORD -56[rbp], rax
   lea		rax, -8[rbp]
   push		rax
   pop		rax
   mov		QWORD -48[rbp], rax
   lea		rax, -12[rbp]
   push		rax
   pop		rax
   mov		QWORD -40[rbp], rax
   push		3
   lea		rax, -56[rbp]
   push		rax
   pop		rax
   pop		rcx
   mov		QWORD -32[rbp], rax
   mov		QWORD -24[rbp], rcx
   mov		rax, 0
   push		rax
   pop		rax
   imul		rax, 8
   push		rax
   pop		rax
   mov		rbx, QWORD -32[rbp]
   add		rbx, rax
   mov		rbx, [rbx]
   push		rbx
   lea		rax, -4[rbp]
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 12
   call		assert
   mov		rax, 1
   push		rax
   pop		rax
   imul		rax, 8
   push		rax
   pop		rax
   mov		rbx, QWORD -32[rbp]
   add		rbx, rax
   mov		rbx, [rbx]
   push		rbx
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 13
   call		assert
   mov		rax, 2
   push		rax
   pop		rax
   imul		rax, 8
   push		rax
   pop		rax
   mov		rbx, QWORD -32[rbp]
   add		rbx, rax
   mov		rbx, [rbx]
   push		rbx
   lea		rax, -12[rbp]
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 14
   call		assert

   ; initialization of 'p0'
   mov		QWORD -64[rbp], 0
   mov		rax, 0
   push		rax
   pop		rax
   imul		rax, 8
   push		rax
   pop		rax
   mov		rbx, QWORD -32[rbp]
   add		rbx, rax
   mov		rbx, [rbx]
   push		rbx
   pop		rax
   mov		QWORD -64[rbp], rax

   ; initialization of 'p1'
   mov		QWORD -72[rbp], 0
   mov		rax, 1
   push		rax
   pop		rax
   imul		rax, 8
   push		rax
   pop		rax
   mov		rbx, QWORD -32[rbp]
   add		rbx, rax
   mov		rbx, [rbx]
   push		rbx
   pop		rax
   mov		QWORD -72[rbp], rax

   ; initialization of 'p2'
   mov		QWORD -80[rbp], 0
   mov		rax, 2
   push		rax
   pop		rax
   imul		rax, 8
   push		rax
   pop		rax
   mov		rbx, QWORD -32[rbp]
   add		rbx, rax
   mov		rbx, [rbx]
   push		rbx
   pop		rax
   mov		QWORD -80[rbp], rax
   mov		rax, -64[rbp]
   push		rax
   lea		rax, -4[rbp]
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 21
   call		assert
   mov		rax, -72[rbp]
   push		rax
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 22
   call		assert
   mov		rax, -80[rbp]
   push		rax
   lea		rax, -12[rbp]
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 23
   call		assert
   mov		rax, 0
   push		rax
   pop		rax
   imul		rax, 8
   push		rax
   pop		rax
   mov		rbx, QWORD -32[rbp]
   add		rbx, rax
   push		rbx
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 25
   call		assert
   mov		rax, 1
   push		rax
   pop		rax
   imul		rax, 8
   push		rax
   pop		rax
   mov		rbx, QWORD -32[rbp]
   add		rbx, rax
   push		rbx
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 2
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 26
   call		assert
   mov		rax, 2
   push		rax
   pop		rax
   imul		rax, 8
   push		rax
   pop		rax
   mov		rbx, QWORD -32[rbp]
   add		rbx, rax
   push		rbx
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 3
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 27
   call		assert

   ; expression of print
   mov		rax, 1
   push		rax
   pop		rax
   imul		rax, 8
   push		rax
   pop		rax
   mov		rbx, QWORD -32[rbp]
   add		rbx, rax
   push		rbx

   ; call to print
   pop		rdx
   mov		rcx, fmt_address
   call		printf

   ; expression of print
   mov		rax, 2
   push		rax
   pop		rax
   imul		rax, 8
   push		rax
   pop		rax
   mov		rbx, QWORD -32[rbp]
   add		rbx, rax
   push		rbx

   ; call to print
   pop		rdx
   mov		rcx, fmt_address
   call		printf
   mov		rax, QWORD -32[rbp]
   mov		rbx, QWORD -24[rbp]
   push		rbx
   push		rax
   ; For-loop
   pop		rax
   pop		rbx
   mov		-96[rbp], rax     ; data
   mov		-104[rbp], rbx     ; count
   mov		QWORD -112[rbp], 0 ; index
L1:
   mov		rbx, -104[rbp]
   mov		rax, -112[rbp]
   cmp		rax, rbx
   jge		L2
   mov		rbx, -96[rbp]
   imul		rax, 8
   add		rbx, rax
   mov		rax, QWORD [rbx]
   mov		-88[rbp], rax 

   ; expression of print
   mov		rax, -88[rbp]
   push		rax
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax

   ; call to print
   pop		rdx
   mov		rcx, fmt_int
   call		printf
   inc		QWORD -112[rbp]
   jmp		L1
L2:
L0:
   mov		rax, 0
   add		rsp, 144
   pop		rbp
   ret
