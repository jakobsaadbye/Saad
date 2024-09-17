bits 64
default rel

segment .data
   fmt_int   DB "%lld", 10, 0
   fmt_float DB "%lf", 10, 0
   fmt_string DB "%s", 10, 0
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
   sub		rsp, 48

   ; initialization of 'ptr'
   mov		QWORD -8[rbp], 0
L0:
   mov		rax, 0
   add		rsp, 48
   pop		rbp
   ret
