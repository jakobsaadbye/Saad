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
   CS0 DB "Mike", 0 
   CS1 DB "Solomon", 0 
   CS2 DB "Ralph", 0 
   CS3 DB "", 0 
   CS4 DB "ugly", 0 
   CS5 DB "not ugly", 0 
   CS6 DB "", 0 
   CS7 DB "scary", 0 
   CS8 DB "not scary", 0 
   CS9 DB `My name is %s, i am %s and %s`, 10, 0 
   __HTTPStatus.ok DB "HTTPStatus.ok", 0
   __HTTPStatus.bad_request DB "HTTPStatus.bad_request", 0
   __HTTPStatus.unauthorized DB "HTTPStatus.unauthorized", 0

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


main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 160

   ; initialization of 'monsters'
   mov		QWORD -16[rbp], 0
   mov		QWORD -8[rbp], 0
   mov		QWORD -64[rbp], 0
   mov		BYTE -56[rbp], 0
   mov		BYTE -55[rbp], 0
   mov		QWORD -48[rbp], 0
   mov		BYTE -40[rbp], 0
   mov		BYTE -39[rbp], 0
   mov		QWORD -32[rbp], 0
   mov		BYTE -24[rbp], 0
   mov		BYTE -23[rbp], 0
   mov		rax, CS0
   push		rax
   pop		rax
   mov		QWORD -64[rbp], rax
   push		-1
   pop		rax
   mov		BYTE -56[rbp], al
   push		0
   pop		rax
   mov		BYTE -55[rbp], al
   mov		rax, CS1
   push		rax
   pop		rax
   mov		QWORD -48[rbp], rax
   push		0
   pop		rax
   mov		BYTE -40[rbp], al
   push		-1
   pop		rax
   mov		BYTE -39[rbp], al
   mov		rax, CS2
   push		rax
   pop		rax
   mov		QWORD -32[rbp], rax
   push		-1
   pop		rax
   mov		BYTE -24[rbp], al
   push		-1
   pop		rax
   mov		BYTE -23[rbp], al
   push		3
   lea		rax, -64[rbp]
   push		rax
   pop		rax
   pop		rcx
   mov		QWORD -16[rbp], rax
   mov		QWORD -8[rbp], rcx
   mov		rax, QWORD -16[rbp]
   mov		rbx, QWORD -8[rbp]
   push		rbx
   push		rax
   ; For-loop
   pop		rax
   pop		rbx
   mov		-88[rbp], rax     ; data
   mov		-96[rbp], rbx     ; count
   mov		QWORD -104[rbp], 0 ; index
L1:
   mov		rbx, -96[rbp]
   mov		rax, -104[rbp]
   cmp		rax, rbx
   jge		L3
   mov		rbx, -88[rbp]
   imul		rax, 16
   add		rbx, rax
   mov		rax, [rbx + 0]
   mov		-80[rbp], rax
   mov		rax, [rbx + 8]
   mov		-72[rbp], rax

   ; initialization of 'is_or_is_not_ugly'
   mov		QWORD -112[rbp], 0
   mov		rax, CS3
   push		rax
   pop		rax
   mov		QWORD -112[rbp], rax
   mov		al, BYTE -72[rbp]
   push		rax
   pop		rax
   cmp		al, 0
   jz			L5
   ; block of if
   mov		rax, CS4
   push		rax
   pop		rax
   mov		QWORD -112[rbp], rax
   jmp L4
; else
L5:
   mov		rax, CS5
   push		rax
   pop		rax
   mov		QWORD -112[rbp], rax
   jmp L4
; done
L4:

   ; initialization of 'is_or_is_not_scary'
   mov		QWORD -120[rbp], 0
   mov		rax, CS6
   push		rax
   pop		rax
   mov		QWORD -120[rbp], rax
   mov		al, BYTE -71[rbp]
   push		rax
   pop		rax
   cmp		al, 0
   jz			L7
   ; block of if
   mov		rax, CS7
   push		rax
   pop		rax
   mov		QWORD -120[rbp], rax
   jmp L6
; else
L7:
   mov		rax, CS8
   push		rax
   pop		rax
   mov		QWORD -120[rbp], rax
   jmp L6
; done
L6:

   ; expression of print
   mov		rax, QWORD -80[rbp]
   push		rax
   mov		rax, QWORD -112[rbp]
   push		rax
   mov		rax, QWORD -120[rbp]
   push		rax
   pop		r9
   pop		r8
   pop		rdx
   mov		rcx, CS9
   call		printf
L2:
   inc		QWORD -104[rbp]
   jmp		L1
L3:
L0:
   mov		rax, 0
   add		rsp, 160
   sub		rsp, 32
L4:
   mov		rax, 0
   add		rsp, 32
   pop		rbp
   ret
