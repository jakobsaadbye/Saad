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
   CS0 DB `counter is %d`, 10, 0 
segment .text
   global main
   extern printf
   extern sprintf
   extern ExitProcess
   extern malloc
   extern InitWindow
   extern WindowShouldClose


assert:
   cmp		cl, 0
   jz 		assert_fail
   ret
assert_fail:
   mov		rcx, string_assert_fail
   call		printf
   mov		rcx, 1
   call		ExitProcess


; bytes locals   : 5
; bytes temp     : 0
; bytes total    : 48
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48

   ; Ln 7: $counter = -4
   mov		rax, 0
   push		rax
   pop		rax
   mov		DWORD -4[rbp], eax

   ; Ln 8: $should_quit = -5
   push		0
   pop		rax
   mov		BYTE -5[rbp], al
L1:
   mov		al, BYTE -5[rbp]
   push		rax
   pop		rax
   test		rax, rax
   sete		al
   movzx		rax, al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L2
   ; While body
   mov		rax, 1
   push		rax
   pop		rax
   add		-4[rbp], eax
   mov		eax, DWORD -4[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 10
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L3
   ; block of if
   jmp		L2
   jmp L3
; done
L3:
   mov		eax, DWORD -4[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 5
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L4
   ; block of if
   jmp		L1
   jmp L4
; done
L4:

   ; expression of print
   mov		eax, DWORD -4[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   call		printf
   jmp			L1
L2:
L0:
   mov		rax, 0
   add		rsp, 48
   pop		rbp
   ret
