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
   CS0 DB `{ r = %lld, g = %lld, b = %lld, a = %lld }`, 10, 0 
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


; bytes locals   : 24
; bytes temp     : 0
; bytes total    : 64
DrawText:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64
   mov		-8[rbp], rcx
   mov		-12[rbp], edx
   mov		-16[rbp], r8d
   mov		-20[rbp], r9d
   mov		rax, [rbp+48]
   mov		-24[rbp], eax
   lea		rax, -24[rbp]
   push		rax
   jmp		L0
L0:
   pop		rax
   mov		rax, [rax]
   add		rsp, 64
   pop		rbp
   ret

; bytes locals   : 4
; bytes temp     : 8
; bytes total    : 48
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48

   ; Ln 13: $color = -4
   mov		rax, 40
   push		rax
   pop		rax
   mov		BYTE -4[rbp], al
   mov		rax, 60
   push		rax
   pop		rax
   mov		BYTE -3[rbp], al
   mov		rax, 80
   push		rax
   pop		rax
   mov		BYTE -2[rbp], al
   mov		rax, 100
   push		rax
   pop		rax
   mov		BYTE -1[rbp], al

   ; expression of print
   lea		rax, -4[rbp]
   push		rax
   pop		r9
   lea		rbx, 0[r9]
   mov		al, BYTE [rbx]
   push		rax
   lea		rbx, 1[r9]
   mov		al, BYTE [rbx]
   push		rax
   lea		rbx, 2[r9]
   mov		al, BYTE [rbx]
   push		rax
   lea		rbx, 3[r9]
   mov		al, BYTE [rbx]
   push		rax
   pop		rax
   movzx		rax, al
   mov		QWORD -32[rbp], rax
   pop		rax
   movzx		r9, al
   pop		rax
   movzx		r8, al
   pop		rax
   movzx		rdx, al
   mov		rcx, CS0
   mov		rax, -32[rbp]
   mov		QWORD [rsp + 32], rax
   call		printf
L1:
   mov		rax, 0
   add		rsp, 48
   pop		rbp
   ret
