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
   __DogKind.samoid DB "samoid", 0
   __DogKind.chiwawa DB "chiwawa", 0
   __DogKind.bulldog DB "bulldog", 0
   CF0 DD 20.0000000
   CF1 DD 0.5000000
   CF2 DD 80.0000000
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

get_enum_string_DogKind:
   mov		r8, 0
   cmp		rdx, r8
   jz			enum_case_0
   mov		r8, 1
   cmp		rdx, r8
   jz			enum_case_1
   mov		r8, 2
   cmp		rdx, r8
   jz			enum_case_2
   push		rcx
   mov		r8, rdx
   mov		rdx, fmt_int
   push		r8
   push		rdx
   push		rcx
   call		sprintf
   pop		rax
   pop		rbx
   pop		rbx
   pop		rbx
   ret
enum_case_0:
   mov		rax, __DogKind.samoid
   ret
enum_case_1:
   mov		rax, __DogKind.chiwawa
   ret
enum_case_2:
   mov		rax, __DogKind.bulldog
   ret

; bytes locals   : 24
; bytes temp     : 16
; bytes total    : 80
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 80
   ; Ln 15: $samoid : Dog = -16[rbp]
   lea		rcx, -16[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   push		0
   pop		rax
   mov		DWORD -16[rbp], eax
   mov		rax, 4
   push		rax
   pop		rax
   mov		DWORD -12[rbp], eax
   movss		xmm0, [CF0]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-8[rbp], eax
   movss		xmm0, [CF1]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-4[rbp], eax
   ; Ln 22: $bmi : float = -20[rbp]
   lea		rax, -16[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		eax, [rbx]
   push		rax
   lea		rax, -16[rbp]
   push		rax
   pop		rbx
   add		rbx, 12
   push		rbx
   pop		rbx
   mov		eax, [rbx]
   push		rax
   lea		rax, -16[rbp]
   push		rax
   pop		rbx
   add		rbx, 12
   push		rbx
   pop		rbx
   mov		eax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   mulss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   divss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-20[rbp], eax
   mov		eax, -20[rbp]
   push		rax
   movss		xmm0, [CF2]
   movd		eax, xmm0
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   comiss	xmm0, xmm1
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 23
   call		assert
   lea		rax, -16[rbp]
   push		rax
   pop		rbx
   add		rbx, 4
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 4
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 24
   call		assert
L4:
   mov		rax, 0
   add		rsp, 80
   pop		rbp
   ret
