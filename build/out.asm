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
   __Coffee.ARABICA DB ".ARABICA", 0
   __Coffee.SPECIALTY DB ".SPECIALTY", 0
   CS0 DB `OK`, 10, 0 
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

get_enum_string_Coffee:
   mov		r8, 0
   cmp		rdx, r8
   jz			enum_case_0
   mov		r8, 1
   cmp		rdx, r8
   jz			enum_case_1
   push		rcx
   mov		r8, rdx
   mov		rdx, fmt_int
   push		r8
   push		rdx
   push		rcx
   call		sprintf
   pop		rax
   pop		rbx
   pop		rbx
   pop		rbx
   ret
enum_case_0:
   mov		rax, __Coffee.ARABICA
   ret
enum_case_1:
   mov		rax, __Coffee.SPECIALTY
   ret

; bytes locals   : 0
; bytes temp     : 0
; bytes total    : 32
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 32
   push		0
   push		1
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setl		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L4
   ; block of if
   ; Ln 9 Print
   ; Pop print arguments
   mov		rcx, CS0
   call		printf
   jmp L4
; done
L4:
L3:
   mov		rax, 0
   add		rsp, 32
   pop		rbp
   ret
