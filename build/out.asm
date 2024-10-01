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
   enum_buffer_0 times 20 DB 0
   enum_buffer_1 times 20 DB 0
   enum_buffer_2 times 20 DB 0
   CS0 DB `Received http status: %s and %s and %s`, 10, 0 

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
   sub		rsp, 64

   ; initialization of 'error1'
   mov		DWORD -4[rbp], 0
   push		200
   pop		rax
   mov		DWORD -4[rbp], eax

   ; initialization of 'error2'
   mov		DWORD -8[rbp], 0
   push		400
   pop		rax
   mov		DWORD -8[rbp], eax

   ; initialization of 'error3'
   mov		DWORD -12[rbp], 0
   push		401
   pop		rax
   mov		DWORD -12[rbp], eax

   ; expression of print
   mov		eax, DWORD -4[rbp]
   push		rax
   mov		eax, DWORD -8[rbp]
   push		rax
   mov		eax, DWORD -12[rbp]
   push		rax
   mov		rax, enum_buffer_0
   pop		rbx
   mov		rcx, rax
   mov		rdx, rbx
   call		print_enum_HTTPStatus
   mov		r9, rax
   mov		rax, enum_buffer_1
   pop		rbx
   push		r9   ; storing
   mov		rcx, rax
   mov		rdx, rbx
   call		print_enum_HTTPStatus
   mov		r8, rax
   pop		r9   ; restore
   mov		rax, enum_buffer_2
   pop		rbx
   push		r9   ; storing
   push		r8   ; storing
   mov		rcx, rax
   mov		rdx, rbx
   call		print_enum_HTTPStatus
   mov		rdx, rax
   pop		r8   ; restore
   pop		r9   ; restore
   mov		rcx, CS0
   call		printf
L4:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret
