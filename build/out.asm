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
   enum_to_int_buffer times 20 DB 0
   CS0 DB `v = %d`, 0 
   CS1 DB `%s %d %d\n`, 0 
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
   extern Type_uint
   extern Type_u8
   extern Type_u16
   extern Type_u32
   extern Type_u64
   extern Type_int
   extern Type_s8
   extern Type_s16
   extern Type_s32
   extern Type_s64
   extern Type_float
   extern Type_f32
   extern Type_f64
   extern Type_string
   extern Type_bool
   extern Type_void
   extern Type_untyped_int
   extern Type_untyped_float


assert:
   cmp		cl, 0
   jz 		assert_fail
   ret
assert_fail:
   mov		rcx, string_assert_fail
   call		printf
   mov		rcx, 1
   call		ExitProcess


; bytes locals   : 16
; bytes temp     : 8
; bytes total    : 64
foo:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64
   ; Param v
   mov		-4[rbp], ecx
   ; Ln 4 Print
   mov		eax, DWORD -4[rbp]
   movsx		rax, eax
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   call		printf
L0:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret

; bytes locals   : 48
; bytes temp     : 8
; bytes total    : 96
bar:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 96
   ; Param a
   mov		-1[rbp], cl
   ; Param b
   mov		-8[rbp], edx
   ; Param c
   mov		-12[rbp], r8d
   ; Ln 8 Print
   movzx		eax, BYTE -1[rbp]
   push		rax
   pop		rax
   cmp		al, 0
   jnz		L2
   mov		rax, string_false
   jmp		L3
L2:
   mov		rax, string_true
L3:
   push		rax
   mov		eax, DWORD -8[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax
   ; Pop print arguments
   pop		rax
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS1
   call		printf
L1:
   mov		rax, 0
   add		rsp, 96
   pop		rbp
   ret

; bytes locals   : 0
; bytes temp     : 48
; bytes total    : 80
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 80
   push		1
   mov		rax, 5
   push		rax
   mov		rax, 8
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   call		bar
   push		1
   mov		rax, 1
   push		rax
   mov		rax, 8
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   call		bar
   push		1
   mov		rax, 1
   push		rax
   mov		rax, 42
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   call		bar
L4:
   mov		rax, 0
   add		rsp, 80
   pop		rbp
   ret
