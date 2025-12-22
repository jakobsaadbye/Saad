bits 64
default rel

segment .data
   fmt_int   DB "%lld", 0
   fmt_float DB "%lf", 0
   fmt_string DB "%s", 0
   fmt_address DB "0x%lx", 0
   string_false DB "false", 0
   string_true  DB "true", 0
   string_assert_fail  DB "Assertion failed at line %d", 10, 0
   CS0 DB "entire file", 0 
   CS1 DB "test.sd", 0 
   CS2 DB `%s`, 10, 0 
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


; bytes locals   : 8
; bytes temp     : 0
; bytes total    : 48
readEntireFile:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48
   ; Copy filepath -> -8
   mov		-8[rbp], rcx
   push		1
   mov		rax, CS0
   push		rax
   jmp		L0
L0:
   pop		rax
   add		rsp, 48
   pop		rbp
   ret

; bytes locals   : 8
; bytes temp     : 8
; bytes total    : 48
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48

   ; Ln 6: $ok : bool = -1
   mov		rax, CS1
   push		rax
   pop		rax
   mov		rcx, rax
   call		readEntireFile
   push		rax
   pop		rax
   mov		BYTE -1[rbp], al
   movzx		eax, BYTE -1[rbp]
   push		rax
   pop		rax
   test		rax, rax
   sete		al
   movzx		rax, al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L2
   ; block of if
   jmp		L1
   jmp L2
; done
L2:
   ; Ln 11 Print
   movzx		eax, BYTE -1[rbp]
   push		rax
   pop		rax
   cmp		al, 0
   jnz		L3
   mov		rax, string_false
   jmp		L4
L3:
   mov		rax, string_true
L4:
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS2
   call		printf
L1:
   mov		rax, 0
   add		rsp, 48
   pop		rbp
   ret
