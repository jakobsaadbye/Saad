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
   CS0 DB `%d %d %d %d %d %d %d %d`, 10, 0 
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


; bytes locals   : 152
; bytes temp     : 0
; bytes total    : 192
add:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 192
   mov		-8[rbp], rcx	; Return (5, int)
   mov		-16[rbp], rdx	; Return (6, int)
   mov		-24[rbp], r8	; Return (7, int)
   ; Copy a -> -28
   mov		-28[rbp], r9d
   ; Copy b -> -32
   mov		rax, 48[rbp]
   mov		-32[rbp], eax
   ; Copy c -> -36
   mov		rax, 56[rbp]
   mov		-36[rbp], eax
   ; Copy d -> -40
   mov		rax, 64[rbp]
   mov		-40[rbp], eax
   ; Copy e -> -44
   mov		rax, 72[rbp]
   mov		-44[rbp], eax
   ; Copy f -> -48
   mov		rax, 80[rbp]
   mov		-48[rbp], eax
   ; Copy g -> -52
   mov		rax, 88[rbp]
   mov		-52[rbp], eax
   ; Copy h -> -56
   mov		rax, 96[rbp]
   mov		-56[rbp], eax
   mov		eax, DWORD -28[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -32[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -36[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -40[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -44[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -48[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -52[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -56[rbp]
   movsx		rax, eax
   push		rax
   jmp		L0
L0:
   mov		rbx, -24[rbp]
   pop		rax
   mov		DWORD [rbx], eax
   mov		rbx, -16[rbp]
   pop		rax
   mov		DWORD [rbx], eax
   mov		rbx, -8[rbp]
   pop		rax
   mov		DWORD [rbx], eax
   pop		r9
   pop		r8
   pop		rdx
   pop		rcx
   pop		rax
   add		rsp, 192
   pop		rbp
   ret

; bytes locals   : 64
; bytes temp     : 64
; bytes total    : 160
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 160
   ; Ln 21: $a : int = -4[rbp]
   mov		rax, 1
   push		rax
   mov		rax, 2
   push		rax
   mov		rax, 3
   push		rax
   mov		rax, 4
   push		rax
   mov		rax, 5
   push		rax
   mov		rax, 6
   push		rax
   mov		rax, 7
   push		rax
   mov		rax, 8
   push		rax
   pop		rax
   mov		-72[rbp], rax
   pop		rax
   mov		-80[rbp], rax
   pop		rax
   mov		-88[rbp], rax
   pop		rax
   mov		-96[rbp], rax
   pop		rax
   mov		-104[rbp], rax
   pop		rax
   mov		-112[rbp], rax
   pop		rax
   mov		-120[rbp], rax
   pop		rax
   mov		r9d, eax
   mov		rax, QWORD -120[rbp]
   mov		32[rsp], eax
   mov		rax, QWORD -112[rbp]
   mov		40[rsp], eax
   mov		rax, QWORD -104[rbp]
   mov		48[rsp], eax
   mov		rax, QWORD -96[rbp]
   mov		56[rsp], eax
   mov		rax, QWORD -88[rbp]
   mov		64[rsp], eax
   mov		rax, QWORD -80[rbp]
   mov		72[rsp], eax
   mov		rax, QWORD -72[rbp]
   mov		80[rsp], eax
   lea		rcx, -68[rbp]		; Return value (5, int)
   lea		rdx, -72[rbp]		; Return value (6, int)
   lea		r8, -76[rbp]		; Return value (7, int)
   call		add
   mov		rbx, -76[rbp]
   push		rbx
   mov		rbx, -72[rbp]
   push		rbx
   mov		rbx, -68[rbp]
   push		rbx
   push		r9
   push		r8
   push		rdx
   push		rcx
   push		rax
   pop		rax
   mov		DWORD -4[rbp], eax
   ; Ln 21: $b : int = -8[rbp]
   pop		rax
   mov		DWORD -8[rbp], eax
   ; Ln 21: $c : int = -12[rbp]
   pop		rax
   mov		DWORD -12[rbp], eax
   ; Ln 21: $d : int = -16[rbp]
   pop		rax
   mov		DWORD -16[rbp], eax
   ; Ln 21: $e : int = -20[rbp]
   pop		rax
   mov		DWORD -20[rbp], eax
   ; Ln 21: $f : int = -24[rbp]
   pop		rax
   mov		DWORD -24[rbp], eax
   ; Ln 21: $g : int = -28[rbp]
   pop		rax
   mov		DWORD -28[rbp], eax
   ; Ln 21: $h : int = -32[rbp]
   pop		rax
   mov		DWORD -32[rbp], eax
   ; Ln 28 Print
   mov		eax, DWORD -4[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -8[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -16[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -20[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -24[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -28[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -32[rbp]
   movsx		rax, eax
   push		rax
   ; Pop print arguments
   pop		rax
   mov		QWORD -72[rbp], rax
   pop		rax
   mov		QWORD -80[rbp], rax
   pop		rax
   mov		QWORD -88[rbp], rax
   pop		rax
   mov		QWORD -96[rbp], rax
   pop		rax
   mov		QWORD -104[rbp], rax
   pop		rax
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   mov		rax, -104[rbp]
   mov		QWORD 32[rsp], rax
   mov		rax, -96[rbp]
   mov		QWORD 40[rsp], rax
   mov		rax, -88[rbp]
   mov		QWORD 48[rsp], rax
   mov		rax, -80[rbp]
   mov		QWORD 56[rsp], rax
   mov		rax, -72[rbp]
   mov		QWORD 64[rsp], rax
   call		printf
L1:
   mov		rax, 0
   add		rsp, 160
   pop		rbp
   ret
