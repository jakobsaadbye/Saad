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
   CS0 DB `%lld`, 10, 0 
   CS1 DB ``, 10, 0 
   CS2 DB `%lld`, 10, 0 
   CS3 DB ``, 10, 0 
   CS4 DB `%lld`, 10, 0 
   CS5 DB ``, 10, 0 
   CS6 DB `%lld`, 10, 0 
   CS7 DB ``, 10, 0 
   CS8 DB `%lld`, 10, 0 
   CS9 DB ``, 10, 0 
   CS10 DB `To infinity and beyond!`, 10, 0 
   CS11 DB ``, 10, 0 
   CS12 DB `(%lld, %lld)`, 10, 0 
   CS13 DB `(%lld, %lld)`, 10, 0 
   CS14 DB ``, 10, 0 

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
   sub		rsp, 352
   mov		rax, 0
   push		rax
   mov		rax, 10
   push		rax
   pop		rbx
   pop		rax
   mov		-16[rbp], rax
   mov		-24[rbp], rbx
   mov		rax, -16[rbp]
   mov		-8[rbp], rax
L1:
   mov		rax, -24[rbp]
   cmp		-8[rbp], rax
   jge		L3

   ; expression of print
   mov		rax, QWORD -8[rbp]
   push		rax
   pop		rdx
   mov		rcx, CS0
   call		printf
L2:
   inc		QWORD -8[rbp]
   jmp		L1
L3:

   ; expression of print
   mov		rcx, CS1
   call		printf
   mov		rax, 0
   push		rax
   mov		rax, 10
   push		rax
   pop		rbx
   pop		rax
   mov		-40[rbp], rax
   mov		-48[rbp], rbx
   mov		rax, -40[rbp]
   mov		-32[rbp], rax
L4:
   mov		rax, -48[rbp]
   cmp		-32[rbp], rax
   jg		L6

   ; expression of print
   mov		rax, QWORD -32[rbp]
   push		rax
   pop		rdx
   mov		rcx, CS2
   call		printf
L5:
   inc		QWORD -32[rbp]
   jmp		L4
L6:

   ; expression of print
   mov		rcx, CS3
   call		printf

   ; initialization of 'a'
   mov		DWORD -52[rbp], 0
   mov		rax, 0
   push		rax
   pop		rax
   mov		DWORD -52[rbp], eax

   ; initialization of 'b'
   mov		DWORD -56[rbp], 0
   mov		rax, 10
   push		rax
   pop		rax
   mov		DWORD -56[rbp], eax
   mov		eax, DWORD -52[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -56[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 10
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   pop		rbx
   pop		rax
   mov		-72[rbp], rax
   mov		-80[rbp], rbx
   mov		rax, -72[rbp]
   mov		-64[rbp], rax
L7:
   mov		rax, -80[rbp]
   cmp		-64[rbp], rax
   jg		L9

   ; expression of print
   mov		rax, QWORD -64[rbp]
   push		rax
   pop		rdx
   mov		rcx, CS4
   call		printf
L8:
   inc		QWORD -64[rbp]
   jmp		L7
L9:

   ; expression of print
   mov		rcx, CS5
   call		printf
   mov		rax, 0
   push		rax
   mov		rax, 10
   push		rax
   pop		rbx
   pop		rax
   mov		-96[rbp], rax
   mov		-104[rbp], rbx
   mov		rax, -96[rbp]
   mov		-88[rbp], rax
L10:
   mov		rax, -104[rbp]
   cmp		-88[rbp], rax
   jge		L12

   ; expression of print
   mov		rax, QWORD -88[rbp]
   push		rax
   pop		rdx
   mov		rcx, CS6
   call		printf
L11:
   inc		QWORD -88[rbp]
   jmp		L10
L12:

   ; expression of print
   mov		rcx, CS7
   call		printf

   ; initialization of 'x'
   mov		QWORD -128[rbp], 0
   mov		QWORD -120[rbp], 0
   mov		DWORD -160[rbp], 0
   mov		DWORD -156[rbp], 0
   mov		DWORD -152[rbp], 0
   mov		DWORD -148[rbp], 0
   mov		DWORD -144[rbp], 0
   mov		DWORD -140[rbp], 0
   mov		DWORD -136[rbp], 0
   mov		DWORD -132[rbp], 0
   mov		rax, 1
   push		rax
   pop		rax
   mov		DWORD -160[rbp], eax
   mov		rax, 1
   push		rax
   pop		rax
   mov		DWORD -156[rbp], eax
   mov		rax, 2
   push		rax
   pop		rax
   mov		DWORD -152[rbp], eax
   mov		rax, 3
   push		rax
   pop		rax
   mov		DWORD -148[rbp], eax
   mov		rax, 5
   push		rax
   pop		rax
   mov		DWORD -144[rbp], eax
   mov		rax, 8
   push		rax
   pop		rax
   mov		DWORD -140[rbp], eax
   mov		rax, 11
   push		rax
   pop		rax
   mov		DWORD -136[rbp], eax
   mov		rax, 19
   push		rax
   pop		rax
   mov		DWORD -132[rbp], eax
   push		8
   lea		rax, -160[rbp]
   push		rax
   pop		rax
   pop		rcx
   mov		QWORD -128[rbp], rax
   mov		QWORD -120[rbp], rcx
   mov		rax, QWORD -128[rbp]
   mov		rbx, QWORD -120[rbp]
   push		rbx
   push		rax
   ; For-loop
   pop		rax
   pop		rbx
   mov		-176[rbp], rax     ; data
   mov		-184[rbp], rbx     ; count
   mov		QWORD -192[rbp], 0 ; index
L13:
   mov		rbx, -184[rbp]
   mov		rax, -192[rbp]
   cmp		rax, rbx
   jge		L15
   mov		rbx, -176[rbp]
   imul		rax, 4
   add		rbx, rax
   mov		eax, DWORD [rbx]
   mov		-168[rbp], eax 

   ; expression of print
   mov		eax, DWORD -168[rbp]
   movsx		rax, eax
   push		rax
   pop		rdx
   mov		rcx, CS8
   call		printf
L14:
   inc		QWORD -192[rbp]
   jmp		L13
L15:

   ; expression of print
   mov		rcx, CS9
   call		printf
L16:

   ; expression of print
   mov		rcx, CS10
   call		printf
   jmp		L18
   jmp		L16
L18:

   ; expression of print
   mov		rcx, CS11
   call		printf

   ; initialization of 'fibonacci_numbers'
   mov		QWORD -208[rbp], 0
   mov		QWORD -200[rbp], 0
   mov		DWORD -240[rbp], 0
   mov		DWORD -236[rbp], 0
   mov		DWORD -232[rbp], 0
   mov		DWORD -228[rbp], 0
   mov		DWORD -224[rbp], 0
   mov		DWORD -220[rbp], 0
   mov		DWORD -216[rbp], 0
   mov		DWORD -212[rbp], 0
   mov		rax, 1
   push		rax
   pop		rax
   mov		DWORD -240[rbp], eax
   mov		rax, 1
   push		rax
   pop		rax
   mov		DWORD -236[rbp], eax
   mov		rax, 2
   push		rax
   pop		rax
   mov		DWORD -232[rbp], eax
   mov		rax, 3
   push		rax
   pop		rax
   mov		DWORD -228[rbp], eax
   mov		rax, 5
   push		rax
   pop		rax
   mov		DWORD -224[rbp], eax
   mov		rax, 8
   push		rax
   pop		rax
   mov		DWORD -220[rbp], eax
   mov		rax, 11
   push		rax
   pop		rax
   mov		DWORD -216[rbp], eax
   mov		rax, 19
   push		rax
   pop		rax
   mov		DWORD -212[rbp], eax
   push		8
   lea		rax, -240[rbp]
   push		rax
   pop		rax
   pop		rcx
   mov		QWORD -208[rbp], rax
   mov		QWORD -200[rbp], rcx
   mov		rax, QWORD -208[rbp]
   mov		rbx, QWORD -200[rbp]
   push		rbx
   push		rax
   ; For-loop
   pop		rax
   pop		rbx
   mov		-256[rbp], rax     ; data
   mov		-264[rbp], rbx     ; count
   mov		QWORD -272[rbp], 0 ; index
L19:
   mov		rbx, -264[rbp]
   mov		rax, -272[rbp]
   cmp		rax, rbx
   jge		L21
   mov		rbx, -256[rbp]
   imul		rax, 4
   add		rbx, rax
   mov		eax, DWORD [rbx]
   mov		-248[rbp], eax 

   ; expression of print
   mov		rax, QWORD -272[rbp]
   push		rax
   mov		eax, DWORD -248[rbp]
   movsx		rax, eax
   push		rax
   pop		r8
   pop		rdx
   mov		rcx, CS12
   call		printf
L20:
   inc		QWORD -272[rbp]
   jmp		L19
L21:
   mov		rax, 10
   push		rax
   mov		rax, 20
   push		rax
   pop		rbx
   pop		rax
   mov		-288[rbp], rax
   mov		-296[rbp], rbx
   mov		rax, -288[rbp]
   mov		-280[rbp], rax
   mov		QWORD -304[rbp], 0
L22:
   mov		rax, -296[rbp]
   cmp		-280[rbp], rax
   jg		L24

   ; expression of print
   mov		rax, QWORD -304[rbp]
   push		rax
   mov		rax, QWORD -280[rbp]
   push		rax
   pop		r8
   pop		rdx
   mov		rcx, CS13
   call		printf
L23:
   inc		QWORD -280[rbp]
   inc		QWORD -304[rbp]
   jmp		L22
L24:

   ; expression of print
   mov		rcx, CS14
   call		printf
L0:
   mov		rax, 0
   add		rsp, 352
   pop		rbp
   ret
