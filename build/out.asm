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
   CS0 DB `%lld`, 10, 0 

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


main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64

   ; initialization of 'N'
   mov		DWORD -4[rbp], 0
   mov		rax, 20
   push		rax
   pop		rax
   mov		DWORD -4[rbp], eax
   mov		rax, 0
   push		rax
   mov		eax, DWORD -4[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   mov		-24[rbp], rax
   mov		-32[rbp], rbx
   mov		rax, -24[rbp]
   mov		-16[rbp], rax
L1:
   mov		rax, -32[rbp]
   cmp		-16[rbp], rax
   jge		L3

   ; expression of print
   mov		rax, QWORD -16[rbp]
   push		rax
   call		fib
   add		rsp, 8
   push		rax
   pop		rdx
   mov		rcx, CS0
   call		printf
L2:
   inc		QWORD -16[rbp]
   jmp		L1
L3:
L0:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret

fib:
   push		rbp
   mov		rbp, rsp
   mov		eax, DWORD 16[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 0
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   mov		eax, DWORD 16[rbp]
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
   jz			L6
   ; block of if
   mov		rax, 1
   push		rax
   jmp		L4
   jmp L5
; else
L6:
   mov		eax, DWORD 16[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   call		fib
   add		rsp, 8
   push		rax
   mov		eax, DWORD 16[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 2
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   call		fib
   add		rsp, 8
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   jmp		L4
   jmp L5
; done
L5:
L4:
   pop		rax
   pop		rbp
   ret
