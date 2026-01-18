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


; bytes locals   : 24
; bytes temp     : 0
; bytes total    : 64
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64
   ; Ln 20: $a : int = -4[rbp]
   mov		rax, 3
   push		rax
   pop		rax
   mov		DWORD -4[rbp], eax
   ; Ln 21: $b : any = -24[rbp]
   mov		eax, DWORD -4[rbp]
   movsx		rax, eax
   push		rax
   mov		rcx, Type_int
   lea		rbx, -24[rbp]
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], rcx
L0:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret
