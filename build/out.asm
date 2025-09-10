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
   CS0 DB `%d = %d`, 10, 0 
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


; bytes locals   : 48
; bytes temp     : 0
; bytes total    : 80
print_ints:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 80
   mov		rax, rcx
   lea		rbx, -16[rbp]
   mov		r8, 16
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy
   ; For-loop
   lea		rax, -16[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-32[rbp], rbx     ; data
   mov		-40[rbp], rcx     ; count
   mov		QWORD -48[rbp], 0 ; index
L1:
   mov		rbx, -40[rbp]
   mov		rax, -48[rbp]
   cmp		rax, rbx
   jge		L3
   mov		rbx, -32[rbp]
   imul		rax, 4
   add		rbx, rax
   mov		eax, DWORD [rbx]
   mov		-24[rbp], eax 

   ; expression of print
   mov		eax, DWORD -48[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -24[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   call		printf
L2:
   inc		QWORD -48[rbp]
   jmp		L1
L3:
L0:
   mov		rax, 0
   add		rsp, 80
   pop		rbp
   ret

; bytes locals   : 36
; bytes temp     : 0
; bytes total    : 80
roll_random_dice:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 80
   mov		-8[rbp], rcx	; Return 0
   mov		-12[rbp], edx

   ; Ln 27: $roll : [..]int = -40
   mov		rdx, 4
   mov		rcx, 8
   call		calloc
   mov		QWORD -40[rbp], rax
   mov		QWORD -32[rbp], 5
   mov		QWORD -24[rbp], 8
   mov		rax, 3
   push		rax
   mov		rax, -40[rbp] ; elem 0
   lea		rbx, [rax + 0]
   pop		rax
   mov		DWORD [rbx], eax
   mov		rax, 27
   push		rax
   mov		rax, -40[rbp] ; elem 1
   lea		rbx, [rax + 4]
   pop		rax
   mov		DWORD [rbx], eax
   mov		rax, 1
   push		rax
   mov		rax, -40[rbp] ; elem 2
   lea		rbx, [rax + 8]
   pop		rax
   mov		DWORD [rbx], eax
   mov		rax, 4
   push		rax
   mov		rax, -40[rbp] ; elem 3
   lea		rbx, [rax + 12]
   pop		rax
   mov		DWORD [rbx], eax
   mov		rax, 5
   push		rax
   mov		rax, -40[rbp] ; elem 4
   lea		rbx, [rax + 16]
   pop		rax
   mov		DWORD [rbx], eax
   lea		rax, -40[rbp]
   push		rax
   jmp		L4
L4:
   pop		rax
   mov		rbx, -8[rbp]
   mov		r8, 16
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy
   mov		rax, -8[rbp]
   add		rsp, 80
   pop		rbp
   ret

; bytes locals   : 16
; bytes temp     : 16
; bytes total    : 64
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64

   ; Ln 32: $roll : []int = -16
   mov		rax, 6
   push		rax
   pop		rax
   mov		edx, eax
   lea		rcx, -32[rbp]		; Return value 0
   call		roll_random_dice
   push		rax
   pop		rax
   mov		rcx, 0[rax]
   mov		rdx, 8[rax]
   mov		-16[rbp], rcx
   mov		-8[rbp], rdx
   lea		rax, -16[rbp]
   push		rax
   pop		rax
   mov		rcx, rax
   call		print_ints
L5:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret
