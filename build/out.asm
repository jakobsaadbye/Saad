bits 64
default rel

segment .data
   fmt_int   DB "%lld", 10, 0
   fmt_float DB "%lf", 10, 0
   fmt_string DB "%s", 10, 0
   fmt_address DB "0x%p", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0
   string_assert_fail  DB "Assertion failed at line %d", 10, 0
   __DogKind.samoid DB "DogKind.samoid", 0
   __DogKind.chiwawa DB "DogKind.chiwawa", 0
   __DogKind.bulldog DB "DogKind.bulldog", 0
   CF0 DD 20.000000
   CF1 DD 0.500000
   CF2 DD 80.000000

segment .text
   global main
   extern printf
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

enum_str:
   mov		rcx, fmt_string
   jmp		enum_end
enum_int:
   mov		rcx, fmt_int
   mov		rdx, rax
enum_end:
   pop		rbx
   call		printf
   push		rbx
   ret

print_enum_DogKind:
   mov		rbx, 0
   cmp		rax, rbx
   jz			enum_case_0
   mov		rbx, 1
   cmp		rax, rbx
   jz			enum_case_2
   mov		rbx, 2
   cmp		rax, rbx
   jz			enum_case_3
   jmp		enum_int
enum_case_0:
   mov		rdx, __DogKind.samoid
   jmp		enum_str
enum_case_2:
   mov		rdx, __DogKind.chiwawa
   jmp		enum_str
enum_case_3:
   mov		rdx, __DogKind.bulldog
   jmp		enum_str

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
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, [rsp]
   add		rsp, 4
   movss		DWORD -8[rbp], xmm0
   movss		xmm0, [CF1]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, [rsp]
   add		rsp, 4
   movss		DWORD -4[rbp], xmm0

   ; initialization of 'bmi'
   mov		DWORD -20[rbp], 0
   movss		xmm0, -8[rbp]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, -4[rbp]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, -4[rbp]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm1, [rsp]
   add		rsp, 4
   movss		xmm0, [rsp]
   add		rsp, 4
   mulss		xmm0, xmm1
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm1, [rsp]
   add		rsp, 4
   movss		xmm0, [rsp]
   add		rsp, 4
   divss		xmm0, xmm1
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, [rsp]
   add		rsp, 4
   movss		DWORD -20[rbp], xmm0
   movss		xmm0, -20[rbp]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm0, [CF2]
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm1, [rsp]
   add		rsp, 4
   movss		xmm0, [rsp]
   add		rsp, 4
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
L5:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret
