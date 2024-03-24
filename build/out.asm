bits 64
default rel

segment .data
   fmt_int   DB "%d", 10, 0
   fmt_float DB "%f", 10, 0
   fmt_string DB "%s", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0
   CF0 DD 2.500000
   CF1 DD 2.500000
   CS2 DB "a is equal to than 2.5", 0 
   CS3 DB "a is greater than 2", 0 

segment .text
   global main
   extern ExitProcess
   extern printf

main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48

   ; initialization of 'a'
   mov		DWORD [rbp - 4], 0
   movss		xmm0, [CF0]
   sub		rsp, 4
   movss		[rsp], xmm0


   ; putting result into 'a'
   movss		xmm0, [rsp]
   add		rsp, 4
   movss		DWORD [rbp - 4], xmm0
   movss		xmm0, [rbp - 4]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, [CF1]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm1, [rsp]
   add		rsp, 4
   movss		xmm0, [rsp]
   add		rsp, 4
   comiss		xmm0, xmm1
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L1
   ; block of if

   ; expression of print
   mov		rax, CS2
   push		rax

   ; call to print
   pop		rdx
   mov		rcx, fmt_string
   call		printf
   jmp L0
;#1 else if
L1:
   movss		xmm0, [rbp - 4]
   sub		rsp, 4
   movss		[rsp], xmm0
   push		2
   pop		rbx
   cvtsi2ss	xmm1, rbx
   movss		xmm0, [rsp]
   add		rsp, 4
   comiss		xmm0, xmm1
   seta		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L0

   ; expression of print
   mov		rax, CS3
   push		rax

   ; call to print
   pop		rdx
   mov		rcx, fmt_string
   call		printf
jmp L0
; done
L0:

   ; cleanup the stack
   pop		rbp
   add		rsp, 48
   mov		rcx, 0
   call		ExitProcess
