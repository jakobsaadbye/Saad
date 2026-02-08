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
   CS0 DB `Hello world`, 0 
   CS1 DB `%llu `, 0 
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


; bytes locals   : 40
; bytes temp     : 8
; bytes total    : 80
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 80
   ; Ln 17: $myString : string = -8[rbp]
   mov		rax, CS0
   push		rax
   pop		rax
   mov		QWORD -8[rbp], rax
   ; Ln 19: Ranged for-loop
   mov		rax, 0
   push		rax
   mov		rax, 11
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD -24[rbp], rax
   mov		QWORD -32[rbp], rbx
   mov		eax, DWORD -24[rbp]
   mov		-16[rbp], eax
L1:
   mov		eax, -32[rbp]
   cmp		-16[rbp], eax
   jge		L3
   ; Ln 20: $c : u8 = -33[rbp]
   mov		rax, QWORD -8[rbp]
   push		rax
   mov		eax, DWORD -16[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   imul		rax, 1; Add index expr
   add		rbx, rax
   movzx		eax, BYTE [rbx]
   push		rax
   pop		rax
   mov		BYTE -33[rbp], al
   ; Ln 21 Print
   movzx		eax, BYTE -33[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   movzx		rax, al
   mov		rdx, rax
   mov		rcx, CS1
   call		printf
L2:
   inc		DWORD -16[rbp]
   jmp		L1
L3:
L0:
   mov		rax, 0
   add		rsp, 80
   pop		rbp
   ret
