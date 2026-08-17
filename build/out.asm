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
	dd 5.000000
	_data_0 DB `%f-%f-%f-%f-%f-%f\n`, 0
	align 8
	STRING_1:
	dq _data_0
	dq 19:
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
foo:
	push        rbp
	mov         rbp, rsp
	sub         rsp, 48
	mov         -8[rbp], rcx
L0:
	mov         rax, 0
	add         rsp, 48
	pop         rbp
	ret         
main:
	push        rbp
	mov         rbp, rsp
	sub         rsp, 32
L1:
	movss       xmm0, [rel FLOAT_0]
	movss       xmm1, xmm0
	mov         rax, STRING_1
	mov         rcx, [rax]
	cvtss2sd    xmm4, xmm1
	movsd       [rsp+32], xmm4
	cvtss2sd    xmm4, xmm1
	movsd       [rsp+40], xmm4
	cvtss2sd    xmm4, xmm1
	movsd       [rsp+48], xmm4
	cvtss2sd    xmm2, xmm1
	cvtss2sd    xmm3, xmm1
	cvtss2sd    xmm1, xmm1
	movq        rdx, xmm1
	movq        r8, xmm2
	movq        r9, xmm3
	call        printf
	mov         rax, 0
	add         rsp, 32
	pop         rbp
	ret         

