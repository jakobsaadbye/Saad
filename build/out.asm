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
	align 4
	FLOAT_4:
	dd 1.000000
	align 4
	FLOAT_5:
	dd 2.000000
	align 4
	FLOAT_6:
	dd 3.000000
	align 4
	FLOAT_7:
	dd 4.000000
	_data_0 DB `%lf`, 0
	align 8
	STRING_8:
	dq _data_0
	dq 3:
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
	sub         rsp, 256
L0:
	lea         rcx, -104[rbp]
	mov         rdx, 0
	mov         r8, 104
	call        memset
	mov         rax, 100
	mov         -104[rbp], eax
	movss       xmm0, [rel FLOAT_0]
	movss       -100[rbp], xmm0
	movss       xmm0, [rel FLOAT_1]
	movss       -96[rbp], xmm0
	movss       xmm0, [rel FLOAT_2]
	movss       -92[rbp], xmm0
	movss       xmm0, [rel FLOAT_3]
	movss       -88[rbp], xmm0
	movss       xmm0, [rel FLOAT_4]
	movss       -84[rbp], xmm0
	movss       xmm0, [rel FLOAT_5]
	movss       -80[rbp], xmm0
	movss       xmm0, [rel FLOAT_6]
	movss       -76[rbp], xmm0
	movss       xmm0, [rel FLOAT_7]
	movss       -72[rbp], xmm0
	lea         rax, -104[rbp]
	lea         rcx, -208[rbp]
	mov         rcx, rcx
	mov         rdx, rax
	mov         r8, 104
	call        memcpy
	lea         rax, -188[rbp]
	mov         rcx, 1
	mov         rdx, rcx
	imul        rdx, 8
	mov         rcx, rax
	add         rcx, rdx
	movss       xmm0, 0[rcx]
	movss       -212[rbp], xmm0
	mov         rax, STRING_8
	mov         rcx, [rax]
	movss       xmm0, -212[rbp]
	cvtss2sd    xmm1, xmm0
	movq        rdx, xmm1
	call        printf
	mov         rax, 0
	add         rsp, 256
	pop         rbp
	ret         

