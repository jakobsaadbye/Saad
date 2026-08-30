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
	dd 7.500000
	align 4
	FLOAT_1:
	dd 10.000000
	_data_0 DB ".\test.sd", 0
	align 8
	STRING_2:
	dq _data_0
	dq 9:
	align 4
	FLOAT_3:
	dd 10.000000
	_data_1 DB ".\test.sd", 0
	align 8
	STRING_4:
	dq _data_1
	dq 9:
	align 4
	FLOAT_5:
	dd 2.500000
	_data_2 DB ".\test.sd", 0
	align 8
	STRING_6:
	dq _data_2
	dq 9:
	align 4
	FLOAT_7:
	dd 12.500000
	_data_3 DB ".\test.sd", 0
	align 8
	STRING_8:
	dq _data_3
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
   extern printf
main:
	push        rbp
	mov         rbp, rsp
	sub         rsp, 48
L0:
	mov         rax, 10
	mov         -4[rbp], eax
	movss       xmm0, [rel FLOAT_0]
	movss       -8[rbp], xmm0
	mov         rax, 5
	mov         -4[rbp], eax
	movss       xmm0, [rel FLOAT_1]
	movss       -8[rbp], xmm0
	mov         eax, -4[rbp]
	mov         rcx, 5
	cmp         eax, ecx
	sete        dl
	movzx       edx, dl
	mov         rax, STRING_2
	mov         cl, dl
	mov         rdx, rax
	mov         r8, 12
	call        runtime_builtin_assert
	movss       xmm0, -8[rbp]
	movss       xmm1, [rel FLOAT_3]
	ucomiss     xmm0, xmm1
	sete        al
	movzx       eax, al
	mov         rcx, STRING_4
	mov         r11, rcx
	mov         cl, al
	mov         rdx, r11
	mov         r8, 13
	call        runtime_builtin_assert
	mov         eax, -4[rbp]
	mov         rcx, 2
	mov         edx, eax
	add         edx, ecx
	mov         -4[rbp], edx
	movss       xmm0, -8[rbp]
	movss       xmm1, [rel FLOAT_5]
	movss       xmm2, xmm0
	addss       xmm2, xmm1
	movss       -8[rbp], xmm2
	mov         eax, -4[rbp]
	mov         rcx, 7
	cmp         eax, ecx
	sete        dl
	movzx       edx, dl
	mov         rax, STRING_6
	mov         cl, dl
	mov         rdx, rax
	mov         r8, 18
	call        runtime_builtin_assert
	movss       xmm0, -8[rbp]
	movss       xmm1, [rel FLOAT_7]
	ucomiss     xmm0, xmm1
	sete        al
	movzx       eax, al
	mov         rcx, STRING_8
	mov         r11, rcx
	mov         cl, al
	mov         rdx, r11
	mov         r8, 19
	call        runtime_builtin_assert
	mov         rax, 0
	add         rsp, 48
	pop         rbp
	ret         

