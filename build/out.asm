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


; bytes locals   : 368
; bytes temp     : 592
; bytes total    : 992
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 992

   ; Ln 32: $doom : [2]Doom = -368
   lea		rax, -352[rbp]
   mov		QWORD -368[rbp], rax
   mov		QWORD -360[rbp], 2
   lea		rax, -336[rbp]
   mov		QWORD -352[rbp], rax
   mov		QWORD -344[rbp], 2
   lea		rax, -320[rbp]
   mov		QWORD -336[rbp], rax
   mov		QWORD -328[rbp], 2
   lea		rax, -304[rbp]
   mov		QWORD -320[rbp], rax
   mov		QWORD -312[rbp], 2
   mov		rax, 5
   push		rax
   pop		rax
   mov		DWORD -304[rbp], eax
   mov		rax, 10
   push		rax
   pop		rax
   mov		DWORD -300[rbp], eax
   lea		rax, -160[rbp]
   mov		QWORD -176[rbp], rax
   mov		QWORD -168[rbp], 2
   lea		rax, -144[rbp]
   mov		QWORD -160[rbp], rax
   mov		QWORD -152[rbp], 2
   lea		rax, -128[rbp]
   mov		QWORD -144[rbp], rax
   mov		QWORD -136[rbp], 2
   mov		rax, 15
   push		rax
   pop		rax
   mov		DWORD -128[rbp], eax
   mov		rax, 20
   push		rax
   pop		rax
   mov		DWORD -124[rbp], eax

   ; expression of print
   mov		rax, 1
   push		rax
   pop		rax
   imul		rax, 176
   push		rax
   pop		rax
   mov		rbx, QWORD -368[rbp]   ; load pointer to .data
   add		rbx, rax              ; add element offset
   add		rbx, 0
   mov		rax, 0
   push		rax
   pop		rax
   imul		rax, 80
   push		rax
   pop		rax
   mov		rbx, [rbx]       ; dereference .data pointer
   add		rbx, rax              ; add element offset
   add		rbx, 0
   mov		rax, 0
   push		rax
   pop		rax
   imul		rax, 32
   push		rax
   pop		rax
   mov		rbx, [rbx]       ; dereference .data pointer
   add		rbx, rax              ; add element offset
   add		rbx, 0
   mov		rax, 0
   push		rax
   pop		rax
   imul		rax, 8
   push		rax
   pop		rax
   mov		rbx, [rbx]       ; dereference .data pointer
   add		rbx, rax              ; add element offset
   add		rbx, 4
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   call		printf
L0:
   mov		rax, 0
   add		rsp, 992
   pop		rbp
   ret
