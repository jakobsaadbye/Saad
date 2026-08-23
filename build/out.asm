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
	dd 4.000000
	align 4
	FLOAT_1:
	dd 9.000000
	align 4
	FLOAT_2:
	dd 13.000000
	align 4
	FLOAT_3:
	dd 15.000000
	_data_0 DB `%d %d %f %f %f %f\n`, 0
	align 8
	STRING_4:
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
main:
	push        rbp
	mov         rbp, rsp
	sub         rsp, 112
L0:
	lea         rcx, -24[rbp]
	mov         rdx, 0
	mov         r8, 24
	call        memset
	mov         rax, 100
	mov         -24[rbp], eax
	movss       xmm0, [rel FLOAT_0]
	movss       -20[rbp], xmm0
	movss       xmm0, [rel FLOAT_1]
	movss       -16[rbp], xmm0
	movss       xmm0, [rel FLOAT_2]
	movss       -12[rbp], xmm0
	movss       xmm0, [rel FLOAT_3]
	movss       -8[rbp], xmm0
	lea         rax, -24[rbp]
	lea         rcx, -48[rbp]
	mov         rcx, rcx
	mov         rdx, rax
	mov         r8, 24
	call        memcpy
	mov         rax, STRING_4
	mov         rcx, [rax]
	mov         eax, -48[rbp]
	mov         edx, -28[rbp]
	movss       xmm0, -44[rbp]
	movss       xmm1, -40[rbp]
	movss       xmm2, -36[rbp]
	movss       xmm3, -32[rbp]
	cvtss2sd    xmm5, xmm1
	movsd       [rsp+32], xmm5
	cvtss2sd    xmm5, xmm2
	movsd       [rsp+40], xmm5
	cvtss2sd    xmm5, xmm3
	movsd       [rsp+48], xmm5
	cvtss2sd    xmm3, xmm0
	mov         r8, rdx
	movq        r9, xmm3
	mov         rdx, rax
	call        printf
	mov         rax, 0
	add         rsp, 112
	pop         rbp
	ret         

