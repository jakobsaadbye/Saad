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
   CS0 DB `%d`, 10, 0 
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


; bytes locals   : 4
; bytes temp     : 28
; bytes total    : 64
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64

   ; Ln 14: $static : int = -4
   lea		rax, -20[rbp]
   mov		QWORD -36[rbp], rax
   mov		QWORD -28[rbp], 3
   mov		rax, 1
   push		rax
   pop		rax
   mov		DWORD -20[rbp], eax
   mov		rax, 2
   push		rax
   pop		rax
   mov		DWORD -16[rbp], eax
   mov		rax, 3
   push		rax
   pop		rax
   mov		DWORD -12[rbp], eax
   mov		rax, 0
   push		rax
   pop		rax
   imul		rax, 4
   push		rax
   pop		rax
   mov		rbx, QWORD -36[rbp]   ; load pointer to .data
   add		rbx, rax             ; add offset
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -4[rbp], eax

   ; expression of print
   mov		eax, DWORD -4[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   call		printf
L0:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret
