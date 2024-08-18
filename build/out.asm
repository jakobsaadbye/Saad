bits 64
default rel

segment .data
   fmt_int   DB "%d", 10, 0
   fmt_float DB "%f", 10, 0
   fmt_string DB "%s", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0
   string_assert_fail  DB "Assertion failed at line %d", 10, 0
   __DogKind.bulldog DB "DogKind.bulldog", 10, 0
   __DogKind.chiwawa DB "DogKind.chiwawa", 10, 0
   __DogKind.samoid DB "DogKind.samoid", 10, 0

segment .text
   global main
   extern printf
   extern ExitProcess

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
   mov		rbx, 2
   cmp		rax, rbx
   jz			enum_case_0
   mov		rbx, 1
   cmp		rax, rbx
   jz			enum_case_2
   mov		rbx, 0
   cmp		rax, rbx
   jz			enum_case_3
   jmp		enum_int
enum_case_0:
   mov		rdx, __DogKind.bulldog
   jmp		enum_str
enum_case_2:
   mov		rdx, __DogKind.chiwawa
   jmp		enum_str
enum_case_3:
   mov		rdx, __DogKind.samoid
   jmp		enum_str

main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48

   ; initialization of 'dog1'
   mov		DWORD -4[rbp], 0
   push		0
   pop		rax
   mov		DWORD -4[rbp], eax

   ; initialization of 'dog2'
   mov		DWORD -8[rbp], 0
   push		2
   pop		rax
   mov		DWORD -8[rbp], eax
   push		1
   pop		rax
   neg		rax
   push		rax
   push		2
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   pop		rax
   mov		-8[rbp], eax
   mov		eax, DWORD -8[rbp]
   push		rax
   push		1

   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 16
   call		assert
   mov		eax, DWORD -4[rbp]
   push		rax
   push		0

   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 17
   call		assert
L5:
   mov		rax, 0
   add		rsp, 48
   pop		rbp
   ret
