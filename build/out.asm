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
	_data_0 DB "%d", 0
	align 8
	STRING_2:
	dq _data_0
	dq 2:
segment .rdata
segment .rdata
	BLOB_0:
	db 255
	times 2 db 0
	db 255
	BLOB_1:
	times 2 db 0
	db 255
	db 255
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
   extern runtime_builtin_assert
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
   extern printf
main:
	push        rbp
	mov         rbp, rsp
	sub         rsp, 48
L0:
	mov         eax, [BLOB_0]
	mov         -4[rbp], eax
	mov         rax, STRING_2
	mov         rcx, [rax]
	movzx       rax, BYTE -1[rbp]
	mov         rdx, rax
	call        printf
	mov         rax, 0
	add         rsp, 48
	pop         rbp
	ret         

