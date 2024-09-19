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
   CF0 DD 1.000000
   CF1 DD 3.000000
   CF2 DD 1.000000
   CF3 DD 3.000000

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
   sub		rsp, 96

   ; initialization of 'v'
   mov		DWORD -8[rbp], 0
   mov		DWORD -4[rbp], 0

   ; initialization of 'ptr'
   mov		QWORD -16[rbp], 0
   lea		rbx, -8[rbp]
   push		rbx
   pop		rax
   mov		QWORD -16[rbp], rax
   movss		xmm0, [CF0]
   sub		rsp, 4
   movss		[rsp], xmm0
   mov		rbx, -16[rbp]
   movss		xmm0, [rsp]
   add		rsp, 4
   movss		DWORD [rbx], xmm0
   lea		rbx, -4[rbp]
   push		rbx
   pop		rax
   mov		QWORD -16[rbp], rax
   movss		xmm0, [CF1]
   sub		rsp, 4
   movss		[rsp], xmm0
   mov		rbx, -16[rbp]
   movss		xmm0, [rsp]
   add		rsp, 4
   movss		DWORD [rbx], xmm0
   mov		rax, 0
   lea		rbx, -8[rbp]
   movss		xmm0, [rbx]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, [CF2]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm1, [rsp]
   add		rsp, 4
   movss		xmm0, [rsp]
   add		rsp, 4
   comiss	xmm0, xmm1
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 22
   call		assert
   mov		rax, 0
   lea		rbx, -4[rbp]
   movss		xmm0, [rbx]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, [CF3]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm1, [rsp]
   add		rsp, 4
   movss		xmm0, [rsp]
   add		rsp, 4
   comiss	xmm0, xmm1
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 23
   call		assert

   ; initialization of 'n1'
   mov		DWORD -32[rbp], 0
   mov		QWORD -24[rbp], 0

   ; initialization of 'n2'
   mov		DWORD -48[rbp], 0
   mov		QWORD -40[rbp], 0

   ; initialization of 'n3'
   mov		DWORD -64[rbp], 0
   mov		QWORD -56[rbp], 0
   mov		rax, 1
   push		rax
   pop		rax
   mov		DWORD -32[rbp], eax
   mov		rax, 2
   push		rax
   pop		rax
   mov		DWORD -48[rbp], eax
   mov		rax, 37
   push		rax
   pop		rax
   mov		DWORD -64[rbp], eax
   lea		rax, -48[rbp]
   push		rax
   pop		rax
   mov		QWORD -24[rbp], rax
   lea		rax, -64[rbp]
   push		rax
   pop		rax
   mov		QWORD -40[rbp], rax
   lea		rax, -32[rbp]
   push		rax
   pop		rax
   mov		QWORD -56[rbp], rax
   mov		rbx, -24[rbp]
   add		rbx, 8
   mov		rbx, [rbx]
   push		rbx
   lea		rax, -64[rbp]
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 39
   call		assert
L0:
   mov		rax, 0
   add		rsp, 96
   pop		rbp
   ret
