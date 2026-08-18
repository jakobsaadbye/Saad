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
	_data_0 DB `%d-%f-%lf-%d-%lf\n`, 0
	align 8
	STRING_0:
	dq _data_0
	dq 18:
	_data_1 DB `%d-%d\n`, 0
	align 8
	STRING_1:
	dq _data_1
	dq 7:
	align 4
	FLOAT_2:
	dd 2.500000
	align 8
	FLOAT_3:
	dq 3.140000
	align 8
	FLOAT_4:
	dq 6.280000
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
	push        rbx
	sub         rsp, 8
	push        rbp
	mov         rbp, rsp
	sub         rsp, 96
	mov         -4[rbp], ecx
	movss       -8[rbp], xmm1
	movsd       -16[rbp], xmm2
	mov         -17[rbp], r9b
	movsd       xmm5, 64[rbp]
	movsd       -32[rbp], xmm5
L0:
	mov         rax, 69
	mov         ebx, eax
	mov         rax, STRING_0
	mov         rcx, [rax]
	mov         eax, -4[rbp]
	movss       xmm0, -8[rbp]
	movsd       xmm1, -16[rbp]
	movzx       rdx, BYTE -17[rbp]
	movsd       xmm2, -32[rbp]
	mov         [rsp+32], rdx
	movsd       [rsp+40], xmm2
	cvtss2sd    xmm2, xmm0
	movaps      xmm3, xmm1
	mov         rdx, rax
	movq        r8, xmm2
	movq        r9, xmm3
	call        printf
	mov         rax, 3
	mov         ecx, ebx
	add         ecx, eax
	mov         eax, ecx
	mov         rcx, STRING_1
	mov         rdx, [rcx]
	mov         rcx, rdx
	mov         rdx, rbx
	mov         r8, rax
	call        printf
	mov         rax, 0
	add         rsp, 96
	add         rsp, 8
	pop         rbp
	pop         rbx
	ret         
main:
	push        rbp
	mov         rbp, rsp
	sub         rsp, 48
L1:
	mov         rax, 1
	movss       xmm0, [rel FLOAT_2]
	movsd       xmm1, [rel FLOAT_3]
	mov         rcx, 255
	movsd       xmm2, [rel FLOAT_4]
	movsd       [rsp+32], xmm2
	movaps      xmm2, xmm1
	movaps      xmm1, xmm0
	mov         r9, rcx
	mov         rcx, rax
	call        foo
	mov         rax, 0
	add         rsp, 48
	pop         rbp
	ret         

