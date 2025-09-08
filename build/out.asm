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


; bytes locals   : 28
; bytes temp     : 0
; bytes total    : 64
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64
   mov		rax, 0
   push		rax
   push		20
   pop		rbx
   pop		rax
   mov		QWORD -16[rbp], rax
   mov		QWORD -24[rbp], rbx
   mov		eax, DWORD -16[rbp]
   mov		-8[rbp], eax
L1:
   mov		eax, -24[rbp]
   cmp		-8[rbp], eax
   jge		L3

   ; expression of print
   mov		eax, DWORD -8[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		ecx, eax
   call		fib
   push		rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   call		printf
L2:
   inc		DWORD -8[rbp]
   jmp		L1
L3:
L0:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret

; bytes locals   : 4
; bytes temp     : 0
; bytes total    : 48
fib:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48
   mov		-4[rbp], ecx
   mov		eax, DWORD -4[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 0
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   mov		eax, DWORD -4[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax

   pop		rbx
   pop		rax
   or 		al, bl
   push		rax
   pop		rax
   cmp		al, 0
   jz			L5
   ; block of if
   mov		rax, 1
   push		rax
   jmp		L4
   jmp L5
; done
L5:
   mov		eax, DWORD -4[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   pop		rax
   mov		ecx, eax
   call		fib
   push		rax
   mov		eax, DWORD -4[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 2
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   pop		rax
   mov		ecx, eax
   call		fib
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   jmp		L4
L4:
   pop		rax
   add		rsp, 48
   pop		rbp
   ret
