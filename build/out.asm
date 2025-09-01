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


; bytes locals   : 40
; bytes temp     : 0
; bytes total    : 80
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 80

   ; Ln 22: $arr = -16
   mov		rax, 1
   push		rax
   pop		rax
   mov		DWORD -32[rbp], eax
   mov		rax, 2
   push		rax
   pop		rax
   mov		DWORD -28[rbp], eax
   mov		rax, 3
   push		rax
   pop		rax
   mov		DWORD -24[rbp], eax
   push		3
   lea		rax, -32[rbp]
   push		rax
   pop		rax
   pop		rcx
   mov		QWORD -16[rbp], rax
   mov		QWORD -8[rbp], rcx

   ; Ln 24: $x = -36
   mov		rax, 2
   push		rax
   pop		rax
   mov		DWORD -36[rbp], eax

   ; Ln 25: $y = -40
   mov		rax, 4
   push		rax
   pop		rax
   mov		DWORD -40[rbp], eax

   ; Ln 26: $z = -44
   mov		eax, DWORD -36[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -40[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   pop		rax
   mov		DWORD -44[rbp], eax

   ; expression of print
   mov		eax, DWORD -44[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   call		printf
L0:
   mov		rax, 0
   add		rsp, 80
   pop		rbp
   ret
