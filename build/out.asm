bits 64
default rel

segment .data
   fmt_int   DB "%lld", 0
   fmt_float DB "%lf", 0
   fmt_string DB "%s", 0
   fmt_address DB "0x%lx", 0
   string_false DB "false", 0
   string_true  DB "true", 0
   string_assert_fail  DB "Assertion failed at line %d", 10, 0
   CS0 DB `%d %s`, 10, 0 
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


; bytes locals   : 16
; bytes temp     : 8
; bytes total    : 64
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64

   ; Ln 11: $v : Vector2 = -8
   lea		rcx, -8[rbp]
   mov		rdx, 0
   mov		r8, 8
   call		memset
   mov		rax, 3
   push		rax
   pop		rax
   mov		DWORD -8[rbp], eax
   mov		rax, 4
   push		rax
   pop		rax
   mov		DWORD -4[rbp], eax
   ; Ln 13 Print
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   movzx		eax, BYTE 0[rbp]
   push		rax
   pop		rax
   cmp		al, 0
   jnz		L1
   mov		rax, string_false
   jmp		L2
L1:
   mov		rax, string_true
L2:
   push		rax
   ; Pop print arguments
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   call		printf
L0:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret
