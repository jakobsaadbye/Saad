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


; bytes locals   : 24
; bytes temp     : 16
; bytes total    : 80
Color.method_receiving_struct:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 80
   mov		-8[rbp], rdx	; Return 0
   mov		16[rbp], rcx 	; Save arg 0 
   ; Copy c -> -24
   mov		rax, 16[rbp]
   lea		rbx, -24[rbp]
   mov		r8, 16
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy
   lea		rcx, -40[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 5
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   pop		rax
   mov		DWORD -40[rbp], eax
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 4
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 5
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   pop		rax
   mov		DWORD -36[rbp], eax
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 5
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   pop		rax
   mov		DWORD -32[rbp], eax
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 12
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 5
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   pop		rax
   mov		DWORD -28[rbp], eax
   lea		rax, -40[rbp]
   push		rax
   jmp		L0
L0:
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

; bytes locals   : 8
; bytes temp     : 0
; bytes total    : 48
Color.method_receiving_ptr_struct:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48
   ; Copy c -> -8
   mov		-8[rbp], rcx
   ; Ln 13: Assignment
   mov		rax, 1
   push		rax
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
   ; Ln 14: Assignment
   mov		rax, 2
   push		rax
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 4
   push		rbx
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
   ; Ln 15: Assignment
   mov		rax, 3
   push		rax
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
   ; Ln 16: Assignment
   mov		rax, 4
   push		rax
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 12
   push		rbx
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
L1:
   mov		rax, 0
   add		rsp, 48
   pop		rbp
   ret

; bytes locals   : 96
; bytes temp     : 16
; bytes total    : 144
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 144

   ; Ln 20: $white : Color = -16
   lea		rcx, -16[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   mov		rax, 255
   push		rax
   pop		rax
   mov		DWORD -16[rbp], eax
   mov		rax, 255
   push		rax
   pop		rax
   mov		DWORD -12[rbp], eax
   mov		rax, 255
   push		rax
   pop		rax
   mov		DWORD -8[rbp], eax
   mov		rax, 255
   push		rax
   pop		rax
   mov		DWORD -4[rbp], eax

   ; Ln 21: $white_ptr : *Color = -24
   lea		rax, -16[rbp]
   push		rax
   pop		rax
   mov		QWORD -24[rbp], rax

   ; Ln 24: $out1 : Color = -40
   lea		rax, -16[rbp]
   push		rax
   pop		rcx
   lea		rdx, -112[rbp]		; Return value 0
   call		Color.method_receiving_struct
   push		rax
   pop		rax
   ; Copy struct
   lea		rbx, -40[rbp]
   mov		r8, 16
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy

   ; Ln 25: $out2 : Color = -56
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rcx
   lea		rdx, -112[rbp]		; Return value 0
   call		Color.method_receiving_struct
   push		rax
   pop		rax
   ; Copy struct
   lea		rbx, -56[rbp]
   mov		r8, 16
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy
   lea		rax, -40[rbp]
   push		rax
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 250
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 27
   call		assert
   lea		rax, -40[rbp]
   push		rax
   pop		rbx
   add		rbx, 4
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 250
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 28
   call		assert
   lea		rax, -40[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 250
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 29
   call		assert
   lea		rax, -40[rbp]
   push		rax
   pop		rbx
   add		rbx, 12
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 250
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 30
   call		assert
   lea		rax, -56[rbp]
   push		rax
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 250
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 32
   call		assert
   lea		rax, -56[rbp]
   push		rax
   pop		rbx
   add		rbx, 4
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 250
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 33
   call		assert
   lea		rax, -56[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 250
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 34
   call		assert
   lea		rax, -56[rbp]
   push		rax
   pop		rbx
   add		rbx, 12
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 250
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 35
   call		assert

   ; Ln 38: $color1 : Color = -72
   lea		rcx, -72[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   mov		rax, 255
   push		rax
   pop		rax
   mov		DWORD -72[rbp], eax
   mov		rax, 255
   push		rax
   pop		rax
   mov		DWORD -68[rbp], eax
   mov		rax, 255
   push		rax
   pop		rax
   mov		DWORD -64[rbp], eax
   mov		rax, 255
   push		rax
   pop		rax
   mov		DWORD -60[rbp], eax

   ; Ln 39: $color2 : Color = -88
   lea		rcx, -88[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   mov		rax, 255
   push		rax
   pop		rax
   mov		DWORD -88[rbp], eax
   mov		rax, 255
   push		rax
   pop		rax
   mov		DWORD -84[rbp], eax
   mov		rax, 255
   push		rax
   pop		rax
   mov		DWORD -80[rbp], eax
   mov		rax, 255
   push		rax
   pop		rax
   mov		DWORD -76[rbp], eax

   ; Ln 40: $color2ptr : *Color = -96
   lea		rax, -88[rbp]
   push		rax
   pop		rax
   mov		QWORD -96[rbp], rax
   lea		rax, -72[rbp]
   push		rax
   pop		rcx
   call		Color.method_receiving_ptr_struct
   push		rax
   pop		rax
   lea		rax, -96[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rcx
   call		Color.method_receiving_ptr_struct
   push		rax
   pop		rax
   lea		rax, -72[rbp]
   push		rax
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 45
   call		assert
   lea		rax, -72[rbp]
   push		rax
   pop		rbx
   add		rbx, 4
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 2
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 46
   call		assert
   lea		rax, -72[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 3
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 47
   call		assert
   lea		rax, -72[rbp]
   push		rax
   pop		rbx
   add		rbx, 12
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 4
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 48
   call		assert
   lea		rax, -96[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 50
   call		assert
   lea		rax, -96[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 4
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 2
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 51
   call		assert
   lea		rax, -96[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 3
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 52
   call		assert
   lea		rax, -96[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 12
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, 4
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 53
   call		assert
L2:
   mov		rax, 0
   add		rsp, 144
   pop		rbp
   ret
