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
   CS0 DB `Brick{ x = %d, y = %d, color = Color{ r = %lld, g = %lld, b = %lld, a = %lld }, dead = %s }`, 10, 0 
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
; bytes temp     : 32
; bytes total    : 80
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 80

   ; Ln 31: $b : Brick = -16
   lea		rcx, -16[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   mov		rax, 5
   push		rax
   pop		rax
   mov		DWORD -16[rbp], eax
   mov		rax, 10
   push		rax
   pop		rax
   mov		DWORD -12[rbp], eax
   lea		rcx, -8[rbp]
   mov		rdx, 0
   mov		r8, 4
   call		memset
   mov		rax, 127
   push		rax
   pop		rax
   mov		BYTE -8[rbp], al
   mov		rax, 127
   push		rax
   pop		rax
   mov		BYTE -7[rbp], al
   mov		rax, 127
   push		rax
   pop		rax
   mov		BYTE -6[rbp], al
   mov		rax, 255
   push		rax
   pop		rax
   mov		BYTE -5[rbp], al
   push		1
   pop		rax
   mov		BYTE -4[rbp], al
   ; Ln 37 Print
   lea		rax, -16[rbp]
   push		rax
   pop		r9
   lea		rbx, 0[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 4[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 8[r9]
   mov		al, BYTE [rbx]
   push		rax
   lea		rbx, 9[r9]
   mov		al, BYTE [rbx]
   push		rax
   lea		rbx, 10[r9]
   mov		al, BYTE [rbx]
   push		rax
   lea		rbx, 11[r9]
   mov		al, BYTE [rbx]
   push		rax
   lea		rbx, 12[r9]
   mov		al, BYTE [rbx]
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
   mov		QWORD -24[rbp], rax
   pop		rax
   movzx		rax, al
   mov		QWORD -32[rbp], rax
   pop		rax
   movzx		rax, al
   mov		QWORD -40[rbp], rax
   pop		rax
   movzx		rax, al
   mov		QWORD -48[rbp], rax
   pop		rax
   movzx		rax, al
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   mov		rax, -48[rbp]
   mov		QWORD 32[rsp], rax
   mov		rax, -40[rbp]
   mov		QWORD 40[rsp], rax
   mov		rax, -32[rbp]
   mov		QWORD 48[rsp], rax
   mov		rax, -24[rbp]
   mov		QWORD 56[rsp], rax
   call		printf
L0:
   mov		rax, 0
   add		rsp, 80
   pop		rbp
   ret
