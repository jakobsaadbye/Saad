bits 64
default rel

segment .data
   fmt_int   DB "%d", 10, 0
   fmt_float DB "%f", 10, 0
   fmt_string DB "%s", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0

segment .text
   global main
   extern printf
   extern ExitProcess


main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 32

   ; expression of print
   push		1000
   call		fib
   add		rsp, 8
   push		rax

   ; call to print
   pop		rdx
   mov		rcx, fmt_int
   call		printf
L0:
   mov		rax, 0
   add		rsp, 32
   pop		rbp
   ret

fib:
   push		rbp
   mov		rbp, rsp
   mov		eax, DWORD 16[rbp]
   push		rax
   push		0

   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   mov		eax, DWORD 16[rbp]
   push		rax
   push		1

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
   jz			L3
   ; block of if
   push		1
   jmp		L1
   jmp L2
; else
L3:
   mov		eax, DWORD 16[rbp]
   push		rax
   mov		eax, DWORD 16[rbp]
   push		rax
   push		1

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
   jmp		L1
   jmp L2
; done
L2:
L1:
   pop		rax
   pop		rbp
   ret
