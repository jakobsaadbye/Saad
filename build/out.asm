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
   CS0 DB `v = %f\n`, 0 
   CF1 DD 3.1500000
   CF2 DD 1.5000000
   CF3 DD 3.1400000
   CF4 DD 4.1500000
   CS5 DB `%f`, 0 
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


; bytes locals   : 72
; bytes temp     : 8
; bytes total    : 112
foo:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 112
   mov		16[rbp], rcx
   mov		24[rbp], rdx
   ; Param ret_0
   mov		rax, 16[rbp]
   mov		-8[rbp], rax
   ; Param floats
   mov		rax, 24[rbp]
   lea		rbx, -24[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 9: For-loop
   lea		rax, -24[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-36[rbp], rbx     ; data
   mov		-44[rbp], rcx     ; count
   mov		QWORD -52[rbp], 0 ; index
L1:
   mov		rbx, -44[rbp]
   mov		rax, -52[rbp]
   cmp		rax, rbx
   jge		L3
   mov		rbx, QWORD -36[rbp]
   mov		rax, QWORD -52[rbp]
   imul		rax, 4
   lea		rbx, [rbx + rax]
   mov		eax, DWORD [rbx]
   mov		-28[rbp], eax 
   ; Ln 10 Print
   mov		eax, -28[rbp]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   call		printf
L2:
   inc		QWORD -52[rbp]
   jmp		L1
L3:
   ; Return
   movss		xmm0, [CF1]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   mov		DWORD [rbx], eax
   jmp		L0
L0:
   mov		rax, 0
   add		rsp, 112
   pop		rbp
   ret

; bytes locals   : 24
; bytes temp     : 40
; bytes total    : 96
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 96
   ; Ln 16: $a : float = -4[rbp]
   lea		rax, -28[rbp]
   push		rax
   movss		xmm0, [CF2]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-16[rbp], eax
   movss		xmm0, [CF3]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-12[rbp], eax
   movss		xmm0, [CF4]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-8[rbp], eax
   lea		rax, -16[rbp]
   mov		QWORD -44[rbp], rax
   mov		QWORD -36[rbp], 3
   lea		rax, -44[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		QWORD -60[rbp], rbx
   mov		QWORD -52[rbp], rcx
   lea		rax, -60[rbp]
   push		rax
   pop		rdx
   pop		rcx
   call		foo
   mov		eax, -28[rbp]
   push		rax
   pop		rax
   mov		-4[rbp], eax
   ; Ln 17 Print
   mov		eax, -4[rbp]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS5
   call		printf
L4:
   mov		rax, 0
   add		rsp, 96
   pop		rbp
   ret
