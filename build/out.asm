bits 64
default rel

segment .data
   fmt_int   DB "%lld", 0
   fmt_float DB "%lf", 0
   fmt_string DB "%s", 0
   fmt_address DB "0x%p", 0
   string_false DB "false", 0
   string_true  DB "true", 0
   string_assert_fail  DB "Assertion failed at line %d", 10, 0
   __DogKind.samoid DB "DogKind.samoid", 0
   __DogKind.chiwawa DB "DogKind.chiwawa", 0
   __DogKind.bulldog DB "DogKind.bulldog", 0
   CF0 DD 20.0000000
   CF1 DD 0.5000000
   CF2 DD 80.0000000

segment .text
   global main
   extern printf
   extern sprintf
   extern ExitProcess
   extern malloc

assert:
   cmp		cl, 0
   jz 		assert_fail
   ret
assert_fail:
   mov		rcx, string_assert_fail
   call		printf
   mov		rcx, 1
   call		ExitProcess

print_enum_DogKind:
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

main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64

   ; initialization of 'samoid'
   mov		DWORD -16[rbp], 0
   mov		DWORD -12[rbp], 0
   mov		DWORD -8[rbp], 0
   mov		DWORD -4[rbp], 0
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

   ; initialization of 'bmi'
   mov		DWORD -20[rbp], 0
   mov		eax, -8[rbp]
   push		rax
   mov		eax, -4[rbp]
   push		rax
   mov		eax, -4[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd	xmm1, ebx
   movd	xmm0, eax
   mulss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rbx
   pop		rax
   movd	xmm1, ebx
   movd	xmm0, eax
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
   movd	xmm1, ebx
   movd	xmm0, eax
   comiss	xmm0, xmm1
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 23
   call		assert
   mov		eax, DWORD -12[rbp]
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
   add		rsp, 64
   pop		rbp
   ret
