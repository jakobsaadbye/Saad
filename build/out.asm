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
   CS0 DB `%d %d %d %d %d %d %d %d %d %d %d %d`, 10, 0 
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


; bytes locals   : 248
; bytes temp     : 0
; bytes total    : 288
add:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 288
   mov		-8[rbp], rcx	; Return (5, int)
   mov		-16[rbp], rdx	; Return (6, int)
   mov		-24[rbp], r8	; Return (7, int)
   mov		-32[rbp], r9	; Return (8, int)
   mov		rax, 144[rbp]	; Return (9, int)
   mov		-40[rbp], rax	
   mov		rax, 152[rbp]	; Return (10, int)
   mov		-48[rbp], rax	
   mov		rax, 160[rbp]	; Return (11, int)
   mov		-56[rbp], rax	
   ; Copy a -> -60
   mov		rax, 48[rbp]
   mov		-60[rbp], eax
   ; Copy b -> -64
   mov		rax, 56[rbp]
   mov		-64[rbp], eax
   ; Copy c -> -68
   mov		rax, 64[rbp]
   mov		-68[rbp], eax
   ; Copy d -> -72
   mov		rax, 72[rbp]
   mov		-72[rbp], eax
   ; Copy e -> -76
   mov		rax, 80[rbp]
   mov		-76[rbp], eax
   ; Copy f -> -80
   mov		rax, 88[rbp]
   mov		-80[rbp], eax
   ; Copy g -> -84
   mov		rax, 96[rbp]
   mov		-84[rbp], eax
   ; Copy h -> -88
   mov		rax, 104[rbp]
   mov		-88[rbp], eax
   ; Copy i -> -92
   mov		rax, 112[rbp]
   mov		-92[rbp], eax
   ; Copy j -> -96
   mov		rax, 120[rbp]
   mov		-96[rbp], eax
   ; Copy k -> -100
   mov		rax, 128[rbp]
   mov		-100[rbp], eax
   ; Copy l -> -104
   mov		rax, 136[rbp]
   mov		-104[rbp], eax
   mov		eax, DWORD -60[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -64[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -68[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -72[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -76[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -80[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -84[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -88[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -92[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -96[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -100[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -104[rbp]
   movsx		rax, eax
   push		rax
   jmp		L0
L0:
   mov		rbx, -56[rbp]
   pop		rax
   mov		DWORD [rbx], eax
   mov		rbx, -48[rbp]
   pop		rax
   mov		DWORD [rbx], eax
   mov		rbx, -40[rbp]
   pop		rax
   mov		DWORD [rbx], eax
   mov		rbx, -32[rbp]
   pop		rax
   mov		DWORD [rbx], eax
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
   add		rsp, 288
   pop		rbp
   ret

; bytes locals   : 96
; bytes temp     : 96
; bytes total    : 224
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 224
   ; Ln 18: $a : int = -4[rbp]
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
   mov		rax, 9
   push		rax
   mov		rax, 10
   push		rax
   mov		rax, 11
   push		rax
   mov		rax, 12
   push		rax
   pop		rax
   mov		-104[rbp], rax
   pop		rax
   mov		-112[rbp], rax
   pop		rax
   mov		-120[rbp], rax
   pop		rax
   mov		-128[rbp], rax
   pop		rax
   mov		-136[rbp], rax
   pop		rax
   mov		-144[rbp], rax
   pop		rax
   mov		-152[rbp], rax
   pop		rax
   mov		-160[rbp], rax
   pop		rax
   mov		-168[rbp], rax
   pop		rax
   mov		-176[rbp], rax
   pop		rax
   mov		-184[rbp], rax
   pop		rax
   mov		-192[rbp], rax
   mov		rax, QWORD -192[rbp]
   mov		32[rsp], eax
   mov		rax, QWORD -184[rbp]
   mov		40[rsp], eax
   mov		rax, QWORD -176[rbp]
   mov		48[rsp], eax
   mov		rax, QWORD -168[rbp]
   mov		56[rsp], eax
   mov		rax, QWORD -160[rbp]
   mov		64[rsp], eax
   mov		rax, QWORD -152[rbp]
   mov		72[rsp], eax
   mov		rax, QWORD -144[rbp]
   mov		80[rsp], eax
   mov		rax, QWORD -136[rbp]
   mov		88[rsp], eax
   mov		rax, QWORD -128[rbp]
   mov		96[rsp], eax
   mov		rax, QWORD -120[rbp]
   mov		104[rsp], eax
   mov		rax, QWORD -112[rbp]
   mov		112[rsp], eax
   mov		rax, QWORD -104[rbp]
   mov		120[rsp], eax
   lea		rcx, -100[rbp]		; Return value (5, int)
   lea		rdx, -104[rbp]		; Return value (6, int)
   lea		r8, -108[rbp]		; Return value (7, int)
   lea		r9, -112[rbp]		; Return value (8, int)
   lea		rax, -116[rbp]		; Return value (9, int)
   mov		128[rsp], rax
   lea		rax, -120[rbp]		; Return value (10, int)
   mov		136[rsp], rax
   lea		rax, -124[rbp]		; Return value (11, int)
   mov		144[rsp], rax
   call		add
   mov		rbx, -124[rbp]
   push		rbx
   mov		rbx, -120[rbp]
   push		rbx
   mov		rbx, -116[rbp]
   push		rbx
   mov		rbx, -112[rbp]
   push		rbx
   mov		rbx, -108[rbp]
   push		rbx
   mov		rbx, -104[rbp]
   push		rbx
   mov		rbx, -100[rbp]
   push		rbx
   push		r9
   push		r8
   push		rdx
   push		rcx
   push		rax
   pop		rax
   mov		DWORD -4[rbp], eax
   ; Ln 18: $b : int = -8[rbp]
   pop		rax
   mov		DWORD -8[rbp], eax
   ; Ln 18: $c : int = -12[rbp]
   pop		rax
   mov		DWORD -12[rbp], eax
   ; Ln 18: $d : int = -16[rbp]
   pop		rax
   mov		DWORD -16[rbp], eax
   ; Ln 18: $e : int = -20[rbp]
   pop		rax
   mov		DWORD -20[rbp], eax
   ; Ln 18: $f : int = -24[rbp]
   pop		rax
   mov		DWORD -24[rbp], eax
   ; Ln 18: $g : int = -28[rbp]
   pop		rax
   mov		DWORD -28[rbp], eax
   ; Ln 18: $h : int = -32[rbp]
   pop		rax
   mov		DWORD -32[rbp], eax
   ; Ln 18: $i : int = -36[rbp]
   pop		rax
   mov		DWORD -36[rbp], eax
   ; Ln 18: $j : int = -40[rbp]
   pop		rax
   mov		DWORD -40[rbp], eax
   ; Ln 18: $k : int = -44[rbp]
   pop		rax
   mov		DWORD -44[rbp], eax
   ; Ln 18: $l : int = -48[rbp]
   pop		rax
   mov		DWORD -48[rbp], eax
   ; Ln 19 Print
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
   ; Pop print arguments
   pop		rax
   mov		QWORD -104[rbp], rax
   pop		rax
   mov		QWORD -112[rbp], rax
   pop		rax
   mov		QWORD -120[rbp], rax
   pop		rax
   mov		QWORD -128[rbp], rax
   pop		rax
   mov		QWORD -136[rbp], rax
   pop		rax
   mov		QWORD -144[rbp], rax
   pop		rax
   mov		QWORD -152[rbp], rax
   pop		rax
   mov		QWORD -160[rbp], rax
   pop		rax
   mov		QWORD -168[rbp], rax
   pop		rax
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   mov		rax, -168[rbp]
   mov		QWORD 32[rsp], rax
   mov		rax, -160[rbp]
   mov		QWORD 40[rsp], rax
   mov		rax, -152[rbp]
   mov		QWORD 48[rsp], rax
   mov		rax, -144[rbp]
   mov		QWORD 56[rsp], rax
   mov		rax, -136[rbp]
   mov		QWORD 64[rsp], rax
   mov		rax, -128[rbp]
   mov		QWORD 72[rsp], rax
   mov		rax, -120[rbp]
   mov		QWORD 80[rsp], rax
   mov		rax, -112[rbp]
   mov		QWORD 88[rsp], rax
   mov		rax, -104[rbp]
   mov		QWORD 96[rsp], rax
   call		printf
L1:
   mov		rax, 0
   add		rsp, 224
   pop		rbp
   ret
