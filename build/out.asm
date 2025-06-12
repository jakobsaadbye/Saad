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
<<<<<<< HEAD
   __DogKind.samoid DB ".samoid", 0
   __DogKind.chiwawa DB ".chiwawa", 0
   __DogKind.bulldog DB ".bulldog", 0
   CF0 DD 20.0000000
   CF1 DD 0.5000000
   CF2 DD 80.0000000
=======
   CF0 DD 1234.1234568
   CF1 DQ 1234.123456789000102
   CS2 DB `%f`, 10, 0 
   CS3 DB `%f`, 10, 0 
   CS4 DB `%lf`, 10, 0 
>>>>>>> 8e175f92326432b8a95f6f9fd2ec11d43b94fc6a

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

<<<<<<< HEAD
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
=======
>>>>>>> 8e175f92326432b8a95f6f9fd2ec11d43b94fc6a

; bytes total    : 20
; bytes arguments: 0
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 56

   ; initialization of 'x'
   mov		DWORD -4[rbp], 0
   mov		rax, 0
   push		rax
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-4[rbp], xmm0

   ; initialization of 'y'
   mov		DWORD -8[rbp], 0
   movss		xmm0, [CF0]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-8[rbp], eax

   ; initialization of 'z'
   mov		DWORD -16[rbp], 0
   movsd		xmm0, [CF1]
   movq		rax, xmm0
   push		rax
   pop		rax
   mov		-16[rbp], rax

   ; expression of print
   mov		eax, -4[rbp]
   push		rax
   pop		rdx
   movq		xmm0, rdx
   cvtss2sd	xmm0, xmm0
   movq		rdx, xmm0
   mov		rcx, CS2
   call		printf

   ; expression of print
   mov		eax, -8[rbp]
   push		rax
   pop		rdx
   movq		xmm0, rdx
   cvtss2sd	xmm0, xmm0
   movq		rdx, xmm0
   mov		rcx, CS3
   call		printf

   ; expression of print
   mov		rax, -16[rbp]
   push		rax
<<<<<<< HEAD
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
   movsx	rax, eax
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
=======
   pop		rdx
   mov		rcx, CS4
   call		printf
L0:
>>>>>>> 8e175f92326432b8a95f6f9fd2ec11d43b94fc6a
   mov		rax, 0
   add		rsp, 56
   pop		rbp
   ret
