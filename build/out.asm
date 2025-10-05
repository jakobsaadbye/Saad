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
   CS0 DB `%s`, 10, 0 
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


; bytes locals   : 1
; bytes temp     : 4
; bytes total    : 48
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48

   ; Ln 6: $yes : bool = -1
   lea		rcx, -12[rbp]
   mov		rdx, 0
   mov		r8, 4
   call		memset
   mov		rax, 5
   push		rax
   pop		rax
   mov		DWORD -12[rbp], eax
   mov		eax, -12[rbp]
   push		rax
   pop		rax
   mov		ecx, eax
   call		should_buy
   push		rax
   pop		rax
   mov		BYTE -1[rbp], al
   ; Ln 7 Print
   mov		al, BYTE -1[rbp]
   push		rax
   pop		rax
   cmp		al, 0
   jnz		L1
   mov		rax, string_false
   jmp		L2
L1:
   mov		rax, string_true
L2:
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   call		printf
L0:
   mov		rax, 0
   add		rsp, 48
   pop		rbp
   ret

; bytes locals   : 4
; bytes temp     : 0
; bytes total    : 48
should_buy:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48
   mov		-4[rbp], ecx
   mov		eax, DWORD -4[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 5
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setb		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L4
   ; block of if
   push		1
   jmp		L3
   jmp L4
; done
L4:
   push		0
   jmp		L3
L3:
   pop		rax
   add		rsp, 48
   pop		rbp
   ret
