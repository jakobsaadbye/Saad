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
   CS0 DB "#%02X%02X%02X", 0 
   CS1 DB `Its just 1 bro`, 10, 0 
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
   extern sprintf


assert:
   cmp		cl, 0
   jz 		assert_fail
   ret
assert_fail:
   mov		rcx, string_assert_fail
   call		printf
   mov		rcx, 1
   call		ExitProcess


; bytes locals   : 24
; bytes temp     : 8
; bytes total    : 64
Color.to_hex:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64
   ; Copy c -> -4
   mov		-4[rbp], ecx

   ; Ln 19: $buffer : [16]u8 = -24
   lea		rbx, -24[rbp]
   mov		rcx, rbx
   mov		rdx, 0
   mov		r8, 16
   call		memset
   lea		rax, -24[rbp]
   push		rax
   mov		rax, CS0
   push		rax
   lea		rax, -4[rbp]
   push		rax
   pop		rbx
   add		rbx, 3
   push		rbx
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   lea		rax, -4[rbp]
   push		rax
   pop		rbx
   add		rbx, 2
   push		rbx
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   lea		rax, -4[rbp]
   push		rax
   pop		rbx
   add		rbx, 1
   push		rbx
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   pop		rax
   mov		-32[rbp], rax
   pop		rax
   mov		r9d, eax
   pop		rax
   mov		r8d, eax
   pop		rax
   mov		rdx, rax
   pop		rax
   mov		rcx, rax
   mov		rax, QWORD -32[rbp]
   mov		32[rsp], eax
   call		sprintf
   push		rax
   pop		rax
   lea		rax, -24[rbp]
   push		rax
   jmp		L0
L0:
   pop		rax
   add		rsp, 64
   pop		rbp
   ret

; bytes locals   : 8
; bytes temp     : 8
; bytes total    : 48
Color.method_test_1:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48
   ; Copy c -> -4
   mov		-4[rbp], ecx
   lea		rcx, -12[rbp]
   mov		rdx, 0
   mov		r8, 4
   call		memset
   lea		rax, -4[rbp]
   push		rax
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   mov		rax, 5
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   pop		rax
   mov		BYTE -12[rbp], al
   lea		rax, -4[rbp]
   push		rax
   pop		rbx
   add		rbx, 1
   push		rbx
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   mov		rax, 5
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   pop		rax
   mov		BYTE -11[rbp], al
   lea		rax, -4[rbp]
   push		rax
   pop		rbx
   add		rbx, 2
   push		rbx
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   mov		rax, 5
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   pop		rax
   mov		BYTE -10[rbp], al
   lea		rax, -4[rbp]
   push		rax
   pop		rbx
   add		rbx, 3
   push		rbx
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   mov		rax, 5
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   pop		rax
   mov		BYTE -9[rbp], al
   mov		eax, -12[rbp]
   push		rax
   jmp		L1
L1:
   pop		rax
   add		rsp, 48
   pop		rbp
   ret

; bytes locals   : 8
; bytes temp     : 0
; bytes total    : 48
Color.method_test_2:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48
   ; Copy c -> -8
   mov		-8[rbp], rcx
   ; Ln 30: Assignment
   mov		rax, 1
   push		rax
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   pop		rax
   mov		BYTE [rbx], al
   ; Ln 31: Assignment
   mov		rax, 2
   push		rax
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 1
   push		rbx
   pop		rbx
   pop		rax
   mov		BYTE [rbx], al
   ; Ln 32: Assignment
   mov		rax, 3
   push		rax
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 2
   push		rbx
   pop		rbx
   pop		rax
   mov		BYTE [rbx], al
   ; Ln 33: Assignment
   mov		rax, 4
   push		rax
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 3
   push		rbx
   pop		rbx
   pop		rax
   mov		BYTE [rbx], al
L2:
   mov		rax, 0
   add		rsp, 48
   pop		rbp
   ret

; bytes locals   : 32
; bytes temp     : 8
; bytes total    : 80
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 80

   ; Ln 37: $white : Color = -4
   lea		rcx, -4[rbp]
   mov		rdx, 0
   mov		r8, 4
   call		memset
   mov		rax, 255
   push		rax
   pop		rax
   mov		BYTE -4[rbp], al
   mov		rax, 256
   push		rax
   pop		rax
   mov		BYTE -3[rbp], al
   mov		rax, 255
   push		rax
   pop		rax
   mov		BYTE -2[rbp], al
   mov		rax, 255
   push		rax
   pop		rax
   mov		BYTE -1[rbp], al

   ; Ln 38: $white_ptr : *Color = -16
   lea		rax, -4[rbp]
   push		rax
   pop		rax
   mov		QWORD -16[rbp], rax

   ; Ln 40: $out1 : Color = -20
   lea		rax, -4[rbp]
   push		rax
   pop		rax
   mov		eax, DWORD [rax]
   push		rax
   pop		rcx
   call		Color.method_test_1
   push		rax
   pop		rax
   ; Copy struct
   mov		-20[rbp], eax

   ; Ln 41: $out2 : Color = -24
   lea		rax, -16[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rax
   mov		rax, QWORD [rax]
   push		rax
   pop		rcx
   call		Color.method_test_1
   push		rax
   pop		rax
   ; Copy struct
   mov		-24[rbp], eax
   lea		rax, -20[rbp]
   push		rax
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   mov		rax, 250
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 43
   call		assert
   lea		rax, -20[rbp]
   push		rax
   pop		rbx
   add		rbx, 1
   push		rbx
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   mov		rax, 250
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 44
   call		assert
   lea		rax, -20[rbp]
   push		rax
   pop		rbx
   add		rbx, 2
   push		rbx
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   mov		rax, 250
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 45
   call		assert
   lea		rax, -20[rbp]
   push		rax
   pop		rbx
   add		rbx, 3
   push		rbx
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   mov		rax, 250
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 46
   call		assert
   lea		rax, -4[rbp]
   push		rax
   pop		rcx
   call		Color.method_test_2
   push		rax
   pop		rax
   lea		rax, -4[rbp]
   push		rax
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L4
   ; block of if
   ; Ln 52 Print
   ; Pop print arguments
   mov		rcx, CS1
   call		printf
   jmp L4
; done
L4:
   lea		rax, -4[rbp]
   push		rax
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 55
   call		assert
   lea		rax, -4[rbp]
   push		rax
   pop		rbx
   add		rbx, 1
   push		rbx
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   mov		rax, 2
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 56
   call		assert
   lea		rax, -4[rbp]
   push		rax
   pop		rbx
   add		rbx, 2
   push		rbx
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   mov		rax, 3
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 57
   call		assert
   lea		rax, -4[rbp]
   push		rax
   pop		rbx
   add		rbx, 3
   push		rbx
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   mov		rax, 4
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 58
   call		assert
L3:
   mov		rax, 0
   add		rsp, 80
   pop		rbp
   ret
