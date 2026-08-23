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
	sub         rsp, 576
L0:
	lea         rcx, -264[rbp]
	mov         rdx, 0
	mov         r8, 264
	call        memset
	mov         rax, 100
	mov         -264[rbp], eax
	movss       xmm0, [rel FLOAT_0]
	movss       -260[rbp], xmm0
	movss       xmm0, [rel FLOAT_1]
	movss       -256[rbp], xmm0
	movss       xmm0, [rel FLOAT_2]
	movss       -252[rbp], xmm0
	movss       xmm0, [rel FLOAT_3]
	movss       -248[rbp], xmm0
	lea         rax, -264[rbp]
	lea         rcx, -528[rbp]
	mov         rcx, rcx
	mov         rdx, rax
	mov         r8, 264
	call        memcpy
	lea         rax, -508[rbp]
	mov         rcx, 1
	mov         rdx, rcx
	imul        rdx, 24
	mov         rcx, rax
	add         rcx, rdx
	mov         rax, 2
	mov         rdx, rax
	imul        rdx, 8
	mov         rax, rcx
	add         rax, rdx
	movss       xmm0, 0[rax]
	movss       -532[rbp], xmm0
	mov         rax, 0
	add         rsp, 576
	pop         rbp
	ret         

