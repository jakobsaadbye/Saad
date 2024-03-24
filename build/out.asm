bits 64
default rel

segment .data
   fmt_int   DB "%d", 10, 0
   fmt_float DB "%f", 10, 0
   fmt_string DB "%s", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0
   CF0 DD 2.500000
   CF1 DD 2.000000
   CF2 DD 6.000000

segment .text
   global main
   extern ExitProcess
   extern printf

main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48

   ; initialization of 'a'
   mov		BYTE [rbp - 1], 0
   push		0

   pop		rax
   not		rax
   push		rax
   push		1

   pop		rbx
   pop		rax
   and		al, bl
   push		rax


   ; putting result into 'a'
   pop		rax
   mov		BYTE [rbp - 1], al

   ; initialization of 'b'
   mov		BYTE [rbp - 2], 0
   push		2
   push		3

   pop		rbx
   pop		rax
   cmp		rax, rbx
   setbe		al
   push		rax
   push		2
   push		4

   pop		rbx
   pop		rax
   cmp		rax, rbx
   setae		al
   push		rax

   pop		rbx
   pop		rax
   and		al, bl
   push		rax
   push		5
   push		6

   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   push		7
   push		3

   pop		rbx
   pop		rax
   cmp		rax, rbx
   setb		al
   push		rax

   pop		rbx
   pop		rax
   and		al, bl
   push		rax
   push		5
   push		9

   pop		rbx
   pop		rax
   cmp		rax, rbx
   seta		al
   push		rax

   pop		rbx
   pop		rax
   and		al, bl
   push		rax

   pop		rbx
   pop		rax
   or		al, bl
   push		rax

   pop		rax
   not		rax
   push		rax


   ; putting result into 'b'
   pop		rax
   mov		BYTE [rbp - 2], al

   ; initialization of 'c'
   mov		BYTE [rbp - 3], 0
   push		5
   push		2

   pop		rbx
   pop		rax
   cmp		rax, rbx
   setne		al
   push		rax


   ; putting result into 'c'
   pop		rax
   mov		BYTE [rbp - 3], al

   ; initialization of 'd'
   mov		BYTE [rbp - 4], 0
   push		0
   push		1

   pop		rbx
   pop		rax
   and		al, bl
   push		rax
   push		3
   push		2

   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax

   pop		rbx
   pop		rax
   or		al, bl
   push		rax


   ; putting result into 'd'
   pop		rax
   mov		BYTE [rbp - 4], al

   ; initialization of 'e'
   mov		BYTE [rbp - 5], 0
   push		0
   push		1

   pop		rbx
   pop		rax
   and		al, bl
   push		rax


   ; putting result into 'e'
   pop		rax
   mov		BYTE [rbp - 5], al

   ; initialization of 'f'
   mov		BYTE [rbp - 6], 0
   movss		xmm0, [CF0]
   sub		rsp, 4
   movss		[rsp], xmm0
   push		1

   pop		rbx
   cvtsi2ss	xmm1, rbx
   movss		xmm0, [rsp]
   add		rsp, 4
   addss		xmm0, xmm1
   sub		rsp, 4
   movss		[rsp], xmm0
   push		4
   pop		rbx
   cvtsi2ss	xmm1, rbx
   movss		xmm0, [rsp]
   add		rsp, 4
   comiss		xmm0, xmm1
   seta		al
   push		rax
   movss		xmm0, [CF1]
   sub		rsp, 4
   movss		[rsp], xmm0
   push		3

   pop		rbx
   cvtsi2ss	xmm1, rbx
   movss		xmm0, [rsp]
   add		rsp, 4
   mulss		xmm0, xmm1
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, [CF2]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm1, [rsp]
   add		rsp, 4
   movss		xmm0, [rsp]
   add		rsp, 4
   comiss		xmm0, xmm1
   sete		al
   push		rax

   pop		rbx
   pop		rax
   or		al, bl
   push		rax


   ; putting result into 'f'
   pop		rax
   mov		BYTE [rbp - 6], al

   ; expression of print
   mov		al, BYTE [rbp - 1]
   push		rax

   ; call to print
   pop		rax
   cmp		al, 0
   jnz		L0
   mov		rcx, string_false
   jmp		L1
L0:
   mov		rcx, string_true
L1:
   call		printf

   ; expression of print
   mov		al, BYTE [rbp - 2]
   push		rax

   ; call to print
   pop		rax
   cmp		al, 0
   jnz		L2
   mov		rcx, string_false
   jmp		L3
L2:
   mov		rcx, string_true
L3:
   call		printf

   ; expression of print
   mov		al, BYTE [rbp - 3]
   push		rax

   ; call to print
   pop		rax
   cmp		al, 0
   jnz		L4
   mov		rcx, string_false
   jmp		L5
L4:
   mov		rcx, string_true
L5:
   call		printf

   ; expression of print
   mov		al, BYTE [rbp - 4]
   push		rax

   ; call to print
   pop		rax
   cmp		al, 0
   jnz		L6
   mov		rcx, string_false
   jmp		L7
L6:
   mov		rcx, string_true
L7:
   call		printf

   ; expression of print
   mov		al, BYTE [rbp - 5]
   push		rax

   ; call to print
   pop		rax
   cmp		al, 0
   jnz		L8
   mov		rcx, string_false
   jmp		L9
L8:
   mov		rcx, string_true
L9:
   call		printf

   ; expression of print
   mov		al, BYTE [rbp - 6]
   push		rax

   ; call to print
   pop		rax
   cmp		al, 0
   jnz		L10
   mov		rcx, string_false
   jmp		L11
L10:
   mov		rcx, string_true
L11:
   call		printf

   ; cleanup the stack
   pop		rbp
   add		rsp, 48
   mov		rcx, 0
   call		ExitProcess
