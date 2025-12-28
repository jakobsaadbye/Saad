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
   CS0 DB `%d %d %d %d %d %d %d %d %d %d`, 10, 0 
   CS1 DB `%d %d %d %d %d %d %d %d %d %d`, 10, 0 
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


; bytes locals   : 200
; bytes temp     : 72
; bytes total    : 304
add:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 304
   mov		-8[rbp], rcx	; Return (5, int)
   mov		-16[rbp], rdx	; Return (6, int)
   mov		-24[rbp], r8	; Return (7, int)
   mov		-32[rbp], r9	; Return (8, int)
   mov		rax, 128[rbp]	; Return (9, int)
   mov		-40[rbp], rax	
   ; Copy a -> -44
   mov		rax, 48[rbp]
   mov		-44[rbp], eax
   ; Copy b -> -48
   mov		rax, 56[rbp]
   mov		-48[rbp], eax
   ; Copy c -> -52
   mov		rax, 64[rbp]
   mov		-52[rbp], eax
   ; Copy d -> -56
   mov		rax, 72[rbp]
   mov		-56[rbp], eax
   ; Copy e -> -60
   mov		rax, 80[rbp]
   mov		-60[rbp], eax
   ; Copy f -> -64
   mov		rax, 88[rbp]
   mov		-64[rbp], eax
   ; Copy g -> -68
   mov		rax, 96[rbp]
   mov		-68[rbp], eax
   ; Copy h -> -72
   mov		rax, 104[rbp]
   mov		-72[rbp], eax
   ; Copy i -> -76
   mov		rax, 112[rbp]
   mov		-76[rbp], eax
   ; Copy j -> -80
   mov		rax, 120[rbp]
   mov		-80[rbp], eax
   ; Ln 21 Print
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
   ; Pop print arguments
   pop		rax
   mov		QWORD -208[rbp], rax
   pop		rax
   mov		QWORD -216[rbp], rax
   pop		rax
   mov		QWORD -224[rbp], rax
   pop		rax
   mov		QWORD -232[rbp], rax
   pop		rax
   mov		QWORD -240[rbp], rax
   pop		rax
   mov		QWORD -248[rbp], rax
   pop		rax
   mov		QWORD -256[rbp], rax
   pop		rax
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   mov		rax, -256[rbp]
   mov		QWORD 32[rsp], rax
   mov		rax, -248[rbp]
   mov		QWORD 40[rsp], rax
   mov		rax, -240[rbp]
   mov		QWORD 48[rsp], rax
   mov		rax, -232[rbp]
   mov		QWORD 56[rsp], rax
   mov		rax, -224[rbp]
   mov		QWORD 64[rsp], rax
   mov		rax, -216[rbp]
   mov		QWORD 72[rsp], rax
   mov		rax, -208[rbp]
   mov		QWORD 80[rsp], rax
   call		printf
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
   jmp		L0
L0:
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
   add		rsp, 304
   pop		rbp
   ret

; bytes locals   : 80
; bytes temp     : 80
; bytes total    : 192
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 192
   ; Ln 30: $a : int = -4[rbp]
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
   mov		-128[rbp], rax
   pop		rax
   mov		-136[rbp], rax
   pop		rax
   mov		-144[rbp], rax
   pop		rax
   mov		-152[rbp], rax
   pop		rax
   mov		-160[rbp], rax
   mov		rax, QWORD -160[rbp]
   mov		32[rsp], eax
   mov		rax, QWORD -152[rbp]
   mov		40[rsp], eax
   mov		rax, QWORD -144[rbp]
   mov		48[rsp], eax
   mov		rax, QWORD -136[rbp]
   mov		56[rsp], eax
   mov		rax, QWORD -128[rbp]
   mov		64[rsp], eax
   mov		rax, QWORD -120[rbp]
   mov		72[rsp], eax
   mov		rax, QWORD -112[rbp]
   mov		80[rsp], eax
   mov		rax, QWORD -104[rbp]
   mov		88[rsp], eax
   mov		rax, QWORD -96[rbp]
   mov		96[rsp], eax
   mov		rax, QWORD -88[rbp]
   mov		104[rsp], eax
   lea		rcx, -84[rbp]		; Return value (5, int)
   lea		rdx, -88[rbp]		; Return value (6, int)
   lea		r8, -92[rbp]		; Return value (7, int)
   lea		r9, -96[rbp]		; Return value (8, int)
   lea		rax, -100[rbp]		; Return value (9, int)
   mov		112[rsp], rax
   call		add
   mov		rbx, -100[rbp]
   push		rbx
   mov		rbx, -96[rbp]
   push		rbx
   mov		rbx, -92[rbp]
   push		rbx
   mov		rbx, -88[rbp]
   push		rbx
   mov		rbx, -84[rbp]
   push		rbx
   push		r9
   push		r8
   push		rdx
   push		rcx
   push		rax
   pop		rax
   mov		DWORD -4[rbp], eax
   ; Ln 30: $b : int = -8[rbp]
   pop		rax
   mov		DWORD -8[rbp], eax
   ; Ln 30: $c : int = -12[rbp]
   pop		rax
   mov		DWORD -12[rbp], eax
   ; Ln 30: $d : int = -16[rbp]
   pop		rax
   mov		DWORD -16[rbp], eax
   ; Ln 30: $e : int = -20[rbp]
   pop		rax
   mov		DWORD -20[rbp], eax
   ; Ln 30: $f : int = -24[rbp]
   pop		rax
   mov		DWORD -24[rbp], eax
   ; Ln 30: $g : int = -28[rbp]
   pop		rax
   mov		DWORD -28[rbp], eax
   ; Ln 30: $h : int = -32[rbp]
   pop		rax
   mov		DWORD -32[rbp], eax
   ; Ln 30: $i : int = -36[rbp]
   pop		rax
   mov		DWORD -36[rbp], eax
   ; Ln 30: $j : int = -40[rbp]
   pop		rax
   mov		DWORD -40[rbp], eax
   ; Ln 37 Print
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
   ; Pop print arguments
   pop		rax
   mov		QWORD -88[rbp], rax
   pop		rax
   mov		QWORD -96[rbp], rax
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
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS1
   mov		rax, -136[rbp]
   mov		QWORD 32[rsp], rax
   mov		rax, -128[rbp]
   mov		QWORD 40[rsp], rax
   mov		rax, -120[rbp]
   mov		QWORD 48[rsp], rax
   mov		rax, -112[rbp]
   mov		QWORD 56[rsp], rax
   mov		rax, -104[rbp]
   mov		QWORD 64[rsp], rax
   mov		rax, -96[rbp]
   mov		QWORD 72[rsp], rax
   mov		rax, -88[rbp]
   mov		QWORD 80[rsp], rax
   call		printf
L1:
   mov		rax, 0
   add		rsp, 192
   pop		rbp
   ret
