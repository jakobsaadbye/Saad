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
   CS0	DB "[10]int", 10, 0
   CS1 DB `%s`, 10, 0 
   CS2 DB `%d`, 10, 0 
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


; bytes locals   : 88
; bytes temp     : 0
; bytes total    : 128
print_ints:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 128
   mov		rax, rcx
   lea		rbx, -56[rbp]
   mov		r8, 16
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy

   ; expression of print
   mov		rax, CS0
   push		rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS1
   call		printf
   ; For-loop
   lea		rax, -56[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-72[rbp], rbx     ; data
   mov		-80[rbp], rcx     ; count
   mov		QWORD -88[rbp], 0 ; index
L1:
   mov		rbx, -80[rbp]
   mov		rax, -88[rbp]
   cmp		rax, rbx
   jge		L3
   mov		rbx, -72[rbp]
   imul		rax, 4
   add		rbx, rax
   mov		eax, DWORD [rbx]
   mov		-64[rbp], eax 

   ; expression of print
   mov		eax, DWORD -64[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS2
   call		printf
L2:
   inc		QWORD -88[rbp]
   jmp		L1
L3:
L0:
   mov		rax, 0
   add		rsp, 128
   pop		rbp
   ret

; bytes locals   : 24
; bytes temp     : 24
; bytes total    : 80
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 80

   ; Ln 28: $nums : [..]int = -24
   mov		rdx, 4
   mov		rcx, 4
   call		calloc
   mov		QWORD -24[rbp], rax
   mov		QWORD -16[rbp], 3
   mov		QWORD -8[rbp], 4
   mov		rax, 1
   push		rax
   mov		rax, -24[rbp] ; elem 0
   lea		rbx, [rax + 0]
   pop		rax
   mov		DWORD [rbx], eax
   mov		rax, 2
   push		rax
   mov		rax, -24[rbp] ; elem 1
   lea		rbx, [rax + 4]
   pop		rax
   mov		DWORD [rbx], eax
   mov		rax, 3
   push		rax
   mov		rax, -24[rbp] ; elem 2
   lea		rbx, [rax + 8]
   pop		rax
   mov		DWORD [rbx], eax
   lea		rax, -24[rbp]
   push		rax
   pop		rax
   mov		rcx, rax
   call		print_ints
L4:
   mov		rax, 0
   add		rsp, 80
   pop		rbp
   ret
