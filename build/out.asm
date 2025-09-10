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
   CS0 DB `Brick{ x = %d, y = %d, dead = %s }`, 10, 0 
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


; bytes locals   : 216
; bytes temp     : 12
; bytes total    : 272
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 272

   ; Ln 14: $bricks : [9]Brick = -128
   lea		rax, -112[rbp]
   mov		QWORD -128[rbp], rax
   mov		QWORD -120[rbp], 9
   mov		rcx, rax
   mov		rdx, 0
   mov		r8, 108
   call		memset
   ; Ln 16: Ranged for-loop
   mov		rax, 0
   push		rax
   mov		rax, 3
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD -144[rbp], rax
   mov		QWORD -152[rbp], rbx
   mov		eax, DWORD -144[rbp]
   mov		-136[rbp], eax
L1:
   mov		eax, -152[rbp]
   cmp		-136[rbp], eax
   jge		L3
   ; Ln 17: Ranged for-loop
   mov		rax, 0
   push		rax
   mov		rax, 3
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD -168[rbp], rax
   mov		QWORD -176[rbp], rbx
   mov		eax, DWORD -168[rbp]
   mov		-160[rbp], eax
L4:
   mov		eax, -176[rbp]
   cmp		-160[rbp], eax
   jge		L6

   ; Ln 18: $index : s32 = -180
   mov		eax, DWORD -136[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 3
   push		rax
   pop		rbx
   pop		rax
   imul		rax, rbx
   push		rax
   mov		eax, DWORD -160[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   pop		rax
   mov		DWORD -180[rbp], eax
   ; Ln 19: Assignment
   mov		DWORD -228[rbp], 0
   mov		DWORD -224[rbp], 0
   mov		BYTE -220[rbp], 0
   mov		eax, DWORD -160[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -228[rbp], eax
   mov		eax, DWORD -136[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -224[rbp], eax
   push		0
   pop		rax
   mov		BYTE -220[rbp], al
   lea		rax, -228[rbp]
   push		rax
   mov		eax, DWORD -180[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   imul		rax, 12
   push		rax
   pop		rax
   mov		rbx, QWORD -128[rbp]   ; load pointer to .data
   add		rbx, rax              ; add element offset
   pop		rax
   ; Copy struct
   mov		r8, 12
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy
L5:
   inc		DWORD -160[rbp]
   jmp		L4
L6:
L2:
   inc		DWORD -136[rbp]
   jmp		L1
L3:
   ; Ln 23: For-loop
   lea		rax, -128[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-204[rbp], rbx     ; data
   mov		-212[rbp], rcx     ; count
   mov		QWORD -220[rbp], 0 ; index
L7:
   mov		rbx, -212[rbp]
   mov		rax, -220[rbp]
   cmp		rax, rbx
   jge		L9
   mov		rbx, -204[rbp]
   imul		rax, 12
   add		rbx, rax
   mov		rax, [rbx + 0]
   mov		-196[rbp], rax
   mov		rax, [rbx + 8]
   mov		-188[rbp], rax

   ; expression of print
   lea		rax, -196[rbp]
   push		rax
   pop		r9
   lea		rbx, 0[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 4[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 8[r9]
   mov		al, BYTE [rbx]
   push		rax
   pop		rax
   cmp		al, 0
   jnz		L10
   mov		rax, string_false
   jmp		L11
L10:
   mov		rax, string_true
L11:
   push		rax
   pop		rax
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   call		printf
L8:
   inc		QWORD -220[rbp]
   jmp		L7
L9:
L0:
   mov		rax, 0
   add		rsp, 272
   pop		rbp
   ret
