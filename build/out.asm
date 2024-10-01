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
   __HTTPStatus.ok DB "HTTPStatus.ok", 0
   __HTTPStatus.bad_request DB "HTTPStatus.bad_request", 0
   __HTTPStatus.unauthorized DB "HTTPStatus.unauthorized", 0
   CS0 DB `0x%p`, 10, 0 
   CS1 DB `0x%p`, 10, 0 

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

print_enum_HTTPStatus:
   mov		r8, 200
   cmp		rdx, r8
   jz			enum_case_0
   mov		r8, 400
   cmp		rdx, r8
   jz			enum_case_1
   mov		r8, 401
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
   mov		rax, __HTTPStatus.ok
   ret
enum_case_1:
   mov		rax, __HTTPStatus.bad_request
   ret
enum_case_2:
   mov		rax, __HTTPStatus.unauthorized
   ret

main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 96

   ; initialization of 'a'
   mov		QWORD -16[rbp], 0
   mov		QWORD -8[rbp], 0
   mov		DWORD -40[rbp], 0
   mov		DWORD -36[rbp], 0
   mov		DWORD -32[rbp], 0
   mov		DWORD -28[rbp], 0
   mov		DWORD -24[rbp], 0
   mov		rax, 1
   push		rax
   pop		rax
   mov		DWORD -40[rbp], eax
   mov		rax, 2
   push		rax
   pop		rax
   mov		DWORD -36[rbp], eax
   mov		rax, 3
   push		rax
   pop		rax
   mov		DWORD -32[rbp], eax
   mov		rax, 4
   push		rax
   pop		rax
   mov		DWORD -28[rbp], eax
   mov		rax, 5
   push		rax
   pop		rax
   mov		DWORD -24[rbp], eax
   push		5
   lea		rax, -40[rbp]
   push		rax
   pop		rax
   pop		rcx
   mov		QWORD -16[rbp], rax
   mov		QWORD -8[rbp], rcx

   ; expression of print
   mov		rax, QWORD -16[rbp]
   mov		rbx, QWORD -8[rbp]
   push		rbx
   push		rax
   pop		rdx
   pop		rbx
   mov		rcx, CS0
   call		printf

   ; expression of print
   mov		rax, 0
   push		rax
   pop		rax
   imul		rax, 4
   push		rax
   pop		rax
   mov		rbx, QWORD -16[rbp]
   add		rbx, rax
   push		rbx
   pop		rdx
   mov		rcx, CS1
   call		printf
L4:
   mov		rax, 0
   add		rsp, 96
   pop		rbp
   ret
