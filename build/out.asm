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
	align 4
	FLOAT_0:
	dd 0.100000
	_data_0 DB ".\test.sd", 0
	align 8
	STRING_1:
	dq _data_0
	dq 9:
segment .rdata
segment .rdata
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
main:
	push        rbp
	mov         rbp, rsp
	sub         rsp, 48
L0:
	movss       xmm0, [rel FLOAT_0]
	mov         rax, xmm0
	imul        rax, -1
	cmp         rax, 0
	setne       cl
	mov         -1[rbp], cl
	movzx       rax, BYTE -1[rbp]
	mov         rcx, STRING_1
	mov         r11, rcx
	mov         cl, al
	mov         rdx, r11
	mov         r8, 8
	call        runtime_builtin_assert
	mov         rax, 0
	add         rsp, 48
	pop         rbp
	ret         

