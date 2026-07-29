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
	align 8
	FLOAT_0:
	dq 3.141591
	align 8
	FLOAT_1:
	dq 2.000000
	_data_0 DB `%lf %lf %lf`, 0
	align 8
	STRING_2:
	dq _data_0
	dq 11:
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
	sub         rsp, 64
L0:
	movsd       xmm0, [rel FLOAT_0]
	movsd       -8[rbp], xmm0
	movsd       xmm0, [rel FLOAT_1]
	movsd       -16[rbp], xmm0
	movsd       xmm0, -8[rbp]
	movsd       xmm1, -16[rbp]
	movsd       xmm2, xmm0
	mulsd       xmm2, xmm1
	movsd       -24[rbp], xmm2
	mov         rbx, STRING_2
	mov         rcx, [rbx]
	movsd       xmm0, -8[rbp]
	movsd       xmm1, -16[rbp]
	movsd       xmm2, -24[rbp]
	movq        rdx, xmm0
	movq        r8, xmm1
	movq        r9, xmm2
	movaps      xmm3, xmm2
	movaps      xmm2, xmm1
	movaps      xmm1, xmm0
	call        printf
	mov         rax, 0
	add         rsp, 64
	pop         rbp
	ret         

