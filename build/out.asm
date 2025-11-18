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
   CS0	DB "*ParticleEmitter", 0
   CS1 DB `%s`, 10, 0 
   CS2 DB `ParticleEmitter{ particles = 0x%p, pos = Vector2{ x = %f, y = %f }, vel = Vector2{ x = %f, y = %f }, radius = %f, color = Color{ r = %lld, g = %lld, b = %lld, a = %lld }, particleLifetime = %d, isActive = %s }`, 10, 0 
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


assert:
   cmp		cl, 0
   jz 		assert_fail
   ret
assert_fail:
   mov		rcx, string_assert_fail
   call		printf
   mov		rcx, 1
   call		ExitProcess


; bytes locals   : 12656
; bytes temp     : 80
; bytes total    : 12768
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 12768

   ; Ln 32: $particleEmitters : [8]ParticleEmitter = -11200
   lea		rbx, -11200[rbp]
   mov		rcx, rbx
   mov		rdx, 0
   mov		r8, 11200
   call		memset
   ; Ln 34: For-loop
   lea		rax, -11200[rbp]
   push		rax
   pop		rax
   mov		rbx, rax
   mov		rcx, 8
   mov		-11216[rbp], rbx     ; data
   mov		-11224[rbp], rcx     ; count
   mov		QWORD -11232[rbp], 0 ; index
L1:
   mov		rbx, -11224[rbp]
   mov		rax, -11232[rbp]
   cmp		rax, rbx
   jge		L3
   mov		rbx, QWORD -11216[rbp]
   mov		rax, QWORD -11232[rbp]
   imul		rax, 1400
   lea		rbx, [rbx + rax]
   mov		-11208[rbp], rbx
   ; Ln 35 Print
   mov		rax, CS0
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS1
   call		printf
   ; Ln 36: Assignment
   lea		rcx, -12660[rbp]
   mov		rdx, 0
   mov		r8, 4
   call		memset
   mov		eax, DWORD -11232[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		BYTE -12660[rbp], al
   mov		eax, DWORD -11232[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		BYTE -12659[rbp], al
   mov		eax, DWORD -11232[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		BYTE -12658[rbp], al
   mov		eax, DWORD -11232[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		BYTE -12657[rbp], al
   mov		eax, -12660[rbp]
   push		rax
   lea		rax, -11208[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 720
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   ; Copy struct
   mov		[rbx], eax
   ; Ln 37: Assignment
   mov		eax, DWORD -11232[rbp]
   movsx		rax, eax
   push		rax
   lea		rax, -11208[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 724
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
L2:
   inc		QWORD -11232[rbp]
   jmp		L1
L3:
   ; Ln 40: For-loop
   lea		rax, -11200[rbp]
   push		rax
   pop		rax
   mov		rbx, rax
   mov		rcx, 8
   mov		-12640[rbp], rbx     ; data
   mov		-12648[rbp], rcx     ; count
   mov		QWORD -12656[rbp], 0 ; index
L4:
   mov		rbx, -12648[rbp]
   mov		rax, -12656[rbp]
   cmp		rax, rbx
   jge		L6
   mov		rbx, QWORD -12640[rbp]
   mov		rax, QWORD -12656[rbp]
   imul		rax, 1400
   lea		rbx, [rbx + rax]
   mov		r8, 1400
   mov		rdx, rbx
   lea		rcx, -12632[rbp]
   call		memcpy
   ; Ln 41 Print
   lea		rax, -12632[rbp]
   push		rax
   pop		r9
   lea		rbx, 0[r9]
   lea		rax, [rbx]
   push		rax
   lea		rbx, 700[r9]
   mov		eax, [rbx]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   lea		rbx, 704[r9]
   mov		eax, [rbx]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   lea		rbx, 708[r9]
   mov		eax, [rbx]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   lea		rbx, 712[r9]
   mov		eax, [rbx]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   lea		rbx, 716[r9]
   mov		eax, [rbx]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   lea		rbx, 720[r9]
   movzx		eax, BYTE [rbx]
   push		rax
   lea		rbx, 721[r9]
   movzx		eax, BYTE [rbx]
   push		rax
   lea		rbx, 722[r9]
   movzx		eax, BYTE [rbx]
   push		rax
   lea		rbx, 723[r9]
   movzx		eax, BYTE [rbx]
   push		rax
   lea		rbx, 724[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 728[r9]
   movzx		eax, BYTE [rbx]
   push		rax
   pop		rax
   cmp		al, 0
   jnz		L7
   mov		rax, string_false
   jmp		L8
L7:
   mov		rax, string_true
L8:
   push		rax
   ; Pop print arguments
   pop		rax
   mov		QWORD -12664[rbp], rax
   pop		rax
   mov		QWORD -12672[rbp], rax
   pop		rax
   movzx		rax, al
   mov		QWORD -12680[rbp], rax
   pop		rax
   movzx		rax, al
   mov		QWORD -12688[rbp], rax
   pop		rax
   movzx		rax, al
   mov		QWORD -12696[rbp], rax
   pop		rax
   movzx		rax, al
   mov		QWORD -12704[rbp], rax
   pop		rax
   mov		QWORD -12712[rbp], rax
   pop		rax
   mov		QWORD -12720[rbp], rax
   pop		rax
   mov		QWORD -12728[rbp], rax
   pop		rax
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS2
   mov		rax, -12728[rbp]
   mov		QWORD 32[rsp], rax
   mov		rax, -12720[rbp]
   mov		QWORD 40[rsp], rax
   mov		rax, -12712[rbp]
   mov		QWORD 48[rsp], rax
   mov		rax, -12704[rbp]
   mov		QWORD 56[rsp], rax
   mov		rax, -12696[rbp]
   mov		QWORD 64[rsp], rax
   mov		rax, -12688[rbp]
   mov		QWORD 72[rsp], rax
   mov		rax, -12680[rbp]
   mov		QWORD 80[rsp], rax
   mov		rax, -12672[rbp]
   mov		QWORD 88[rsp], rax
   mov		rax, -12664[rbp]
   mov		QWORD 96[rsp], rax
   call		printf
L5:
   inc		QWORD -12656[rbp]
   jmp		L4
L6:
L0:
   mov		rax, 0
   add		rsp, 12768
   pop		rbp
   ret
