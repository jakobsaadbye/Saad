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
   CS0 DB "INFINITY!!!", 0 

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

   ; initialization of 'n0'
   mov		QWORD -16[rbp], 0
   mov		DWORD -8[rbp], 0

   ; initialization of 'n1'
   mov		QWORD -32[rbp], 0
   mov		DWORD -24[rbp], 0

   ; initialization of 'n2'
   mov		QWORD -48[rbp], 0
   mov		DWORD -40[rbp], 0
   lea		rax, -32[rbp]
   push		rax
   pop		rax
   mov		QWORD -16[rbp], rax
   lea		rax, -48[rbp]
   push		rax
   pop		rax
   mov		QWORD -32[rbp], rax
   push		0
   pop		rax
   mov		QWORD -48[rbp], rax
   mov		rax, 69
   push		rax
   pop		rax
   mov		DWORD -8[rbp], eax
   mov		rax, 420
   push		rax
   pop		rax
   mov		DWORD -24[rbp], eax
   mov		rax, 1337
   push		rax
   pop		rax
   mov		DWORD -40[rbp], eax

   ; initialization of 'cursor'
   mov		QWORD -56[rbp], 0
   lea		rax, -16[rbp]
   push		rax
   pop		rax
   mov		QWORD -56[rbp], rax
L1:

   ; expression of print
   mov		rax, CS0
   push		rax

   ; call to print
   pop		rdx
   mov		rcx, fmt_string
   call		printf
   jmp		L1
L2:
L0:
   pop		rax
   add		rsp, 96
   pop		rbp
   ret
