bits 64
default rel

segment .data
   fmt_int   DB "%lld", 10, 0
   fmt_float DB "%lf", 10, 0
   fmt_string DB "%s", 10, 0
   fmt_address DB "0x%p", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0
   string_assert_fail  DB "Assertion failed at line %d", 10, 0
   CS0 DB "", 0 
   CS1 DB "", 0 

segment .text
   global main
   extern printf
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

enum_str:
   mov		rcx, fmt_string
   jmp		enum_end
enum_int:
   mov		rcx, fmt_int
   mov		rdx, rax
enum_end:
   pop		rbx
   call		printf
   push		rbx
   ret


main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64
   mov		rax, 1
   push		rax
   pop		rax
   neg		rax
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   mov		-4[rbp], eax
   mov		-8[rbp], ebx
   mov		eax, -4[rbp]
   mov		-12[rbp], eax
L1:
   mov		eax, -8[rbp]
   cmp		-12[rbp], eax
   jg		L3
   mov		rax, 1
   push		rax
   pop		rax
   neg		rax
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   mov		-16[rbp], eax
   mov		-20[rbp], ebx
   mov		eax, -16[rbp]
   mov		-24[rbp], eax
L4:
   mov		eax, -20[rbp]
   cmp		-24[rbp], eax
   jg		L6
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 0
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   mov		eax, DWORD -24[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 0
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax

   pop		rbx
   pop		rax
   and		al, bl
   push		rax
   pop		rax
   cmp		al, 0
   jz			L7
   ; block of if
   jmp		L5
   jmp L7
; done
L7:

   ; expression of print
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax

   ; call to print
   pop		rdx
   mov		rcx, fmt_int
   call		printf

   ; expression of print
   mov		eax, DWORD -24[rbp]
   movsx		rax, eax
   push		rax

   ; call to print
   pop		rdx
   mov		rcx, fmt_int
   call		printf

   ; expression of print
   mov		rax, CS0
   push		rax

   ; call to print
   pop		rdx
   mov		rcx, fmt_string
   call		printf
L5:
   inc		DWORD -24[rbp]
   jmp		L4
L6:

   ; expression of print
   mov		rax, CS1
   push		rax

   ; call to print
   pop		rdx
   mov		rcx, fmt_string
   call		printf
L2:
   inc		DWORD -12[rbp]
   jmp		L1
L3:
L0:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret
