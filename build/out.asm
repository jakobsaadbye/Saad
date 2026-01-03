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
   CS0 DB `%lld\n%f\nVector4{ x = %d, y = %d, z = %d, w = %d }\n%lf\n%s\nVector2{ x = %f, y = %f }\n%s\n0x%p\n%lld\n`, 10, 0 
   CS1 DB `%f %f`, 10, 0 
   CS2 DB `%d %d %d %d %d %d %d %d %d %d %d %d %d`, 10, 0 
   CS3 DB `-------------------------------`, 10, 0 
   CS4 DB `Vector4{ x = %d, y = %d, z = %d, w = %d }\nVector4{ x = %d, y = %d, z = %d, w = %d }\nVector4{ x = %d, y = %d, z = %d, w = %d }\nVector4{ x = %d, y = %d, z = %d, w = %d }\nVector4{ x = %d, y = %d, z = %d, w = %d }`, 10, 0 
   CS5 DB `-------------------------------`, 10, 0 
   CF6 DD 7.5000000
   CF7 DQ 3.140000000000000
   CF8 DD 420.0000000
   CF9 DD 69.0000000
   CS10 DB "This feels like a good mix", 0 
   CS11 DB `%lld\n%f\nVector4{ x = %d, y = %d, z = %d, w = %d }\n%lf\n%s\nVector2{ x = %f, y = %f }\n%s\n0x%p\n%lld\n`, 10, 0 
   CS12 DB `-------------------------------`, 10, 0 
   CF13 DD 3.1400000
   CF14 DD 6.2800000
   CS15 DB `%f %f`, 10, 0 
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


; bytes locals   : 312
; bytes temp     : 0
; bytes total    : 352
many_small_parameters:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 352
   ; Param ret_0
   mov		-8[rbp], rcx
   ; Param ret_1
   mov		-16[rbp], rdx
   ; Param ret_2
   mov		-24[rbp], r8 
   ; Param ret_3
   mov		-32[rbp], r9 
   ; Param ret_4
   mov		rax, 48[rbp]
   mov		-40[rbp], rax
   ; Param ret_5
   mov		rax, 56[rbp]
   mov		-48[rbp], rax
   ; Param ret_6
   mov		rax, 64[rbp]
   mov		-56[rbp], rax
   ; Param ret_7
   mov		rax, 72[rbp]
   mov		-64[rbp], rax
   ; Param ret_8
   mov		rax, 80[rbp]
   mov		-72[rbp], rax
   ; Param ret_9
   mov		rax, 88[rbp]
   mov		-80[rbp], rax
   ; Param ret_10
   mov		rax, 96[rbp]
   mov		-88[rbp], rax
   ; Param ret_11
   mov		rax, 104[rbp]
   mov		-96[rbp], rax
   ; Param ret_12
   mov		rax, 112[rbp]
   mov		-104[rbp], rax
   ; Param a
   mov		rax, 120[rbp]
   mov		-108[rbp], eax
   ; Param b
   mov		rax, 128[rbp]
   mov		-112[rbp], eax
   ; Param c
   mov		rax, 136[rbp]
   mov		-116[rbp], eax
   ; Param d
   mov		rax, 144[rbp]
   mov		-120[rbp], eax
   ; Param e
   mov		rax, 152[rbp]
   mov		-124[rbp], eax
   ; Param f
   mov		rax, 160[rbp]
   mov		-128[rbp], eax
   ; Param g
   mov		rax, 168[rbp]
   mov		-132[rbp], eax
   ; Param h
   mov		rax, 176[rbp]
   mov		-136[rbp], eax
   ; Param i
   mov		rax, 184[rbp]
   mov		-140[rbp], eax
   ; Param j
   mov		rax, 192[rbp]
   mov		-144[rbp], eax
   ; Param k
   mov		rax, 200[rbp]
   mov		-148[rbp], eax
   ; Param l
   mov		rax, 208[rbp]
   mov		-152[rbp], eax
   ; Param m
   mov		rax, 216[rbp]
   mov		-156[rbp], eax
   ; Return
   mov		eax, DWORD -108[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   mov		DWORD [rbx], eax
   mov		eax, DWORD -112[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rbx, -16[rbp]
   mov		DWORD [rbx], eax
   mov		eax, DWORD -116[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rbx, -24[rbp]
   mov		DWORD [rbx], eax
   mov		eax, DWORD -120[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rbx, -32[rbp]
   mov		DWORD [rbx], eax
   mov		eax, DWORD -124[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rbx, -40[rbp]
   mov		DWORD [rbx], eax
   mov		eax, DWORD -128[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rbx, -48[rbp]
   mov		DWORD [rbx], eax
   mov		eax, DWORD -132[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rbx, -56[rbp]
   mov		DWORD [rbx], eax
   mov		eax, DWORD -136[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rbx, -64[rbp]
   mov		DWORD [rbx], eax
   mov		eax, DWORD -140[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rbx, -72[rbp]
   mov		DWORD [rbx], eax
   mov		eax, DWORD -144[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rbx, -80[rbp]
   mov		DWORD [rbx], eax
   mov		eax, DWORD -148[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rbx, -88[rbp]
   mov		DWORD [rbx], eax
   mov		eax, DWORD -152[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rbx, -96[rbp]
   mov		DWORD [rbx], eax
   mov		eax, DWORD -156[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rbx, -104[rbp]
   mov		DWORD [rbx], eax
   jmp		L0
L0:
   mov		rax, 0
   add		rsp, 352
   pop		rbp
   ret

; bytes locals   : 200
; bytes temp     : 0
; bytes total    : 240
many_large_parameters:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 240
   ; Param ret_0
   mov		-8[rbp], rcx
   ; Param ret_1
   mov		-16[rbp], rdx
   ; Param ret_2
   mov		-24[rbp], r8 
   ; Param ret_3
   mov		-32[rbp], r9 
   ; Param ret_4
   mov		rax, 48[rbp]
   mov		-40[rbp], rax
   ; Param a
   mov		rax, 56[rbp]
   lea		rbx, -56[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Param b
   mov		rax, 64[rbp]
   lea		rbx, -72[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Param c
   mov		rax, 72[rbp]
   lea		rbx, -88[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Param d
   mov		rax, 80[rbp]
   lea		rbx, -104[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Param e
   mov		rax, 88[rbp]
   lea		rbx, -120[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Return
   lea		rax, -56[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -72[rbp]
   push		rax
   pop		rax
   mov		rbx, -16[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -88[rbp]
   push		rax
   pop		rax
   mov		rbx, -24[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -104[rbp]
   push		rax
   pop		rax
   mov		rbx, -32[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -120[rbp]
   push		rax
   pop		rax
   mov		rbx, -40[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   jmp		L1
L1:
   mov		rax, 0
   add		rsp, 240
   pop		rbp
   ret

; bytes locals   : 232
; bytes temp     : 96
; bytes total    : 368
many_mixed_parameters:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 368
   ; Param ret_0
   mov		-8[rbp], rcx
   ; Param ret_1
   mov		-16[rbp], rdx
   ; Param ret_2
   mov		-24[rbp], r8 
   ; Param ret_3
   mov		-32[rbp], r9 
   ; Param ret_4
   mov		rax, 48[rbp]
   mov		-40[rbp], rax
   ; Param ret_5
   mov		rax, 56[rbp]
   mov		-48[rbp], rax
   ; Param ret_6
   mov		rax, 64[rbp]
   mov		-56[rbp], rax
   ; Param ret_7
   mov		rax, 72[rbp]
   mov		-64[rbp], rax
   ; Param ret_8
   mov		rax, 80[rbp]
   mov		-72[rbp], rax
   ; Param a
   mov		rax, 88[rbp]
   mov		-73[rbp], al
   ; Param b
   mov		rax, 96[rbp]
   mov		-80[rbp], eax
   ; Param c
   mov		rax, 104[rbp]
   lea		rbx, -96[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Param d
   mov		rax, 112[rbp]
   mov		-104[rbp], rax
   ; Param e
   mov		rax, 120[rbp]
   mov		-105[rbp], al
   ; Param f
   mov		rax, 128[rbp]
   mov		-120[rbp], rax
   ; Param g
   mov		rax, 136[rbp]
   mov		-128[rbp], rax
   ; Param h
   mov		rax, 144[rbp]
   mov		-136[rbp], rax
   ; Param i
   mov		rax, 152[rbp]
   mov		-144[rbp], rax
   ; Ln 19 Print
   movzx		eax, BYTE -73[rbp]
   push		rax
   mov		eax, -80[rbp]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   lea		rax, -96[rbp]
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
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 12[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, -104[rbp]
   push		rax
   movzx		eax, BYTE -105[rbp]
   push		rax
   pop		rax
   cmp		al, 0
   jnz		L3
   mov		rax, string_false
   jmp		L4
L3:
   mov		rax, string_true
L4:
   push		rax
   mov		rax, -120[rbp]
   push		rax
   pop		rax
   mov		-240[rbp], rax
   lea		r9, -240[rbp]
   lea		rbx, 0[r9]
   mov		eax, [rbx]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   lea		rbx, 4[r9]
   mov		eax, [rbx]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   mov		rax, QWORD -128[rbp]
   push		rax
   mov		rax, -136[rbp]
   push		rax
   mov		rax, QWORD -144[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		QWORD -248[rbp], rax
   pop		rax
   mov		QWORD -256[rbp], rax
   pop		rax
   mov		QWORD -264[rbp], rax
   pop		rax
   mov		QWORD -272[rbp], rax
   pop		rax
   mov		QWORD -280[rbp], rax
   pop		rax
   mov		QWORD -288[rbp], rax
   pop		rax
   mov		QWORD -296[rbp], rax
   pop		rax
   mov		QWORD -304[rbp], rax
   pop		rax
   mov		QWORD -312[rbp], rax
   pop		rax
   mov		QWORD -320[rbp], rax
   pop		rax
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   movzx		rax, al
   mov		rdx, rax
   mov		rcx, CS0
   mov		rax, -320[rbp]
   mov		QWORD 32[rsp], rax
   mov		rax, -312[rbp]
   mov		QWORD 40[rsp], rax
   mov		rax, -304[rbp]
   mov		QWORD 48[rsp], rax
   mov		rax, -296[rbp]
   mov		QWORD 56[rsp], rax
   mov		rax, -288[rbp]
   mov		QWORD 64[rsp], rax
   mov		rax, -280[rbp]
   mov		QWORD 72[rsp], rax
   mov		rax, -272[rbp]
   mov		QWORD 80[rsp], rax
   mov		rax, -264[rbp]
   mov		QWORD 88[rsp], rax
   mov		rax, -256[rbp]
   mov		QWORD 96[rsp], rax
   mov		rax, -248[rbp]
   mov		QWORD 104[rsp], rax
   call		printf
   ; Return
   movzx		eax, BYTE -73[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   mov		eax, -80[rbp]
   push		rax
   pop		rax
   mov		rbx, -16[rbp]
   mov		DWORD [rbx], eax
   lea		rax, -96[rbp]
   push		rax
   pop		rax
   mov		rbx, -24[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   mov		rax, -104[rbp]
   push		rax
   pop		rax
   mov		rbx, -32[rbp]
   mov		QWORD [rbx], rax
   movzx		eax, BYTE -105[rbp]
   push		rax
   pop		rax
   mov		rbx, -40[rbp]
   mov		BYTE [rbx], al
   mov		rax, -120[rbp]
   push		rax
   pop		rax
   mov		rbx, -48[rbp]
   mov		QWORD [rbx], rax
   mov		rax, QWORD -128[rbp]
   push		rax
   pop		rax
   mov		rbx, -56[rbp]
   mov		QWORD [rbx], rax
   mov		rax, -136[rbp]
   push		rax
   pop		rax
   mov		rbx, -64[rbp]
   mov		QWORD [rbx], rax
   mov		rax, QWORD -144[rbp]
   push		rax
   pop		rax
   mov		rbx, -72[rbp]
   mov		QWORD [rbx], rax
   jmp		L2
L2:
   mov		rax, 0
   add		rsp, 368
   pop		rbp
   ret

; bytes locals   : 48
; bytes temp     : 8
; bytes total    : 96
float_parameters:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 96
   ; Param ret_0
   mov		-8[rbp], rcx
   ; Param ret_1
   mov		-16[rbp], rdx
   ; Param a
   movd		-20[rbp], xmm2
   ; Param b
   movd		-24[rbp], xmm3
   ; Ln 24 Print
   mov		eax, -20[rbp]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   mov		eax, -24[rbp]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   ; Pop print arguments
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS1
   call		printf
   ; Return
   mov		eax, -20[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   mov		DWORD [rbx], eax
   mov		eax, -24[rbp]
   push		rax
   pop		rax
   mov		rbx, -16[rbp]
   mov		DWORD [rbx], eax
   jmp		L5
L5:
   mov		rax, 0
   add		rsp, 96
   pop		rbp
   ret

; bytes locals   : 376
; bytes temp     : 320
; bytes total    : 736
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 736
   ; Ln 30: $a : int = -4[rbp]
   lea		rax, -380[rbp]
   push		rax
   lea		rax, -384[rbp]
   push		rax
   lea		rax, -388[rbp]
   push		rax
   lea		rax, -392[rbp]
   push		rax
   lea		rax, -396[rbp]
   push		rax
   lea		rax, -400[rbp]
   push		rax
   lea		rax, -404[rbp]
   push		rax
   lea		rax, -408[rbp]
   push		rax
   lea		rax, -412[rbp]
   push		rax
   lea		rax, -416[rbp]
   push		rax
   lea		rax, -420[rbp]
   push		rax
   lea		rax, -424[rbp]
   push		rax
   lea		rax, -428[rbp]
   push		rax
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
   mov		rax, 13
   push		rax
   pop		rax
   mov		-436[rbp], rax
   pop		rax
   mov		-444[rbp], rax
   pop		rax
   mov		-452[rbp], rax
   pop		rax
   mov		-460[rbp], rax
   pop		rax
   mov		-468[rbp], rax
   pop		rax
   mov		-476[rbp], rax
   pop		rax
   mov		-484[rbp], rax
   pop		rax
   mov		-492[rbp], rax
   pop		rax
   mov		-500[rbp], rax
   pop		rax
   mov		-508[rbp], rax
   pop		rax
   mov		-516[rbp], rax
   pop		rax
   mov		-524[rbp], rax
   pop		rax
   mov		-532[rbp], rax
   pop		rax
   mov		-540[rbp], rax
   pop		rax
   mov		-548[rbp], rax
   pop		rax
   mov		-556[rbp], rax
   pop		rax
   mov		-564[rbp], rax
   pop		rax
   mov		-572[rbp], rax
   pop		rax
   mov		-580[rbp], rax
   pop		rax
   mov		-588[rbp], rax
   pop		rax
   mov		-596[rbp], rax
   pop		rax
   mov		-604[rbp], rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   mov		rax, -604[rbp]
   mov		32[rsp], rax
   mov		rax, -596[rbp]
   mov		40[rsp], rax
   mov		rax, -588[rbp]
   mov		48[rsp], rax
   mov		rax, -580[rbp]
   mov		56[rsp], rax
   mov		rax, -572[rbp]
   mov		64[rsp], rax
   mov		rax, -564[rbp]
   mov		72[rsp], rax
   mov		rax, -556[rbp]
   mov		80[rsp], rax
   mov		rax, -548[rbp]
   mov		88[rsp], rax
   mov		rax, -540[rbp]
   mov		96[rsp], rax
   mov		rax, -532[rbp]
   mov		104[rsp], rax
   mov		rax, -524[rbp]
   mov		112[rsp], rax
   mov		rax, -516[rbp]
   mov		120[rsp], rax
   mov		rax, -508[rbp]
   mov		128[rsp], rax
   mov		rax, -500[rbp]
   mov		136[rsp], rax
   mov		rax, -492[rbp]
   mov		144[rsp], rax
   mov		rax, -484[rbp]
   mov		152[rsp], rax
   mov		rax, -476[rbp]
   mov		160[rsp], rax
   mov		rax, -468[rbp]
   mov		168[rsp], rax
   mov		rax, -460[rbp]
   mov		176[rsp], rax
   mov		rax, -452[rbp]
   mov		184[rsp], rax
   mov		rax, -444[rbp]
   mov		192[rsp], rax
   mov		rax, -436[rbp]
   mov		200[rsp], rax
   call		many_small_parameters
   mov		rbx, -428[rbp]
   push		rbx
   mov		rbx, -424[rbp]
   push		rbx
   mov		rbx, -420[rbp]
   push		rbx
   mov		rbx, -416[rbp]
   push		rbx
   mov		rbx, -412[rbp]
   push		rbx
   mov		rbx, -408[rbp]
   push		rbx
   mov		rbx, -404[rbp]
   push		rbx
   mov		rbx, -400[rbp]
   push		rbx
   mov		rbx, -396[rbp]
   push		rbx
   mov		rbx, -392[rbp]
   push		rbx
   mov		rbx, -388[rbp]
   push		rbx
   mov		rbx, -384[rbp]
   push		rbx
   mov		rbx, -380[rbp]
   push		rbx
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
   ; Ln 30: $k : int = -44[rbp]
   pop		rax
   mov		DWORD -44[rbp], eax
   ; Ln 30: $l : int = -48[rbp]
   pop		rax
   mov		DWORD -48[rbp], eax
   ; Ln 30: $m : int = -52[rbp]
   pop		rax
   mov		DWORD -52[rbp], eax
   ; Ln 31 Print
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
   mov		eax, DWORD -52[rbp]
   movsx		rax, eax
   push		rax
   ; Pop print arguments
   pop		rax
   mov		QWORD -384[rbp], rax
   pop		rax
   mov		QWORD -392[rbp], rax
   pop		rax
   mov		QWORD -400[rbp], rax
   pop		rax
   mov		QWORD -408[rbp], rax
   pop		rax
   mov		QWORD -416[rbp], rax
   pop		rax
   mov		QWORD -424[rbp], rax
   pop		rax
   mov		QWORD -432[rbp], rax
   pop		rax
   mov		QWORD -440[rbp], rax
   pop		rax
   mov		QWORD -448[rbp], rax
   pop		rax
   mov		QWORD -456[rbp], rax
   pop		rax
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS2
   mov		rax, -456[rbp]
   mov		QWORD 32[rsp], rax
   mov		rax, -448[rbp]
   mov		QWORD 40[rsp], rax
   mov		rax, -440[rbp]
   mov		QWORD 48[rsp], rax
   mov		rax, -432[rbp]
   mov		QWORD 56[rsp], rax
   mov		rax, -424[rbp]
   mov		QWORD 64[rsp], rax
   mov		rax, -416[rbp]
   mov		QWORD 72[rsp], rax
   mov		rax, -408[rbp]
   mov		QWORD 80[rsp], rax
   mov		rax, -400[rbp]
   mov		QWORD 88[rsp], rax
   mov		rax, -392[rbp]
   mov		QWORD 96[rsp], rax
   mov		rax, -384[rbp]
   mov		QWORD 104[rsp], rax
   call		printf
   ; Ln 33 Print
   ; Pop print arguments
   mov		rcx, CS3
   call		printf
   ; Ln 35: $a : Vector4 = -72[rbp]
   lea		rax, -392[rbp]
   push		rax
   lea		rax, -408[rbp]
   push		rax
   lea		rax, -424[rbp]
   push		rax
   lea		rax, -440[rbp]
   push		rax
   lea		rax, -456[rbp]
   push		rax
   lea		rcx, -472[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   mov		rax, 1
   push		rax
   pop		rax
   mov		DWORD -472[rbp], eax
   mov		rax, 2
   push		rax
   pop		rax
   mov		DWORD -468[rbp], eax
   mov		rax, 3
   push		rax
   pop		rax
   mov		DWORD -464[rbp], eax
   mov		rax, 4
   push		rax
   pop		rax
   mov		DWORD -460[rbp], eax
   lea		rax, -472[rbp]
   push		rax
   lea		rcx, -488[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   mov		rax, 5
   push		rax
   pop		rax
   mov		DWORD -488[rbp], eax
   mov		rax, 6
   push		rax
   pop		rax
   mov		DWORD -484[rbp], eax
   mov		rax, 7
   push		rax
   pop		rax
   mov		DWORD -480[rbp], eax
   mov		rax, 8
   push		rax
   pop		rax
   mov		DWORD -476[rbp], eax
   lea		rax, -488[rbp]
   push		rax
   lea		rcx, -504[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   mov		rax, 9
   push		rax
   pop		rax
   mov		DWORD -504[rbp], eax
   mov		rax, 10
   push		rax
   pop		rax
   mov		DWORD -500[rbp], eax
   mov		rax, 11
   push		rax
   pop		rax
   mov		DWORD -496[rbp], eax
   mov		rax, 12
   push		rax
   pop		rax
   mov		DWORD -492[rbp], eax
   lea		rax, -504[rbp]
   push		rax
   lea		rcx, -520[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   mov		rax, 13
   push		rax
   pop		rax
   mov		DWORD -520[rbp], eax
   mov		rax, 14
   push		rax
   pop		rax
   mov		DWORD -516[rbp], eax
   mov		rax, 15
   push		rax
   pop		rax
   mov		DWORD -512[rbp], eax
   mov		rax, 16
   push		rax
   pop		rax
   mov		DWORD -508[rbp], eax
   lea		rax, -520[rbp]
   push		rax
   lea		rcx, -536[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   mov		rax, 17
   push		rax
   pop		rax
   mov		DWORD -536[rbp], eax
   mov		rax, 18
   push		rax
   pop		rax
   mov		DWORD -532[rbp], eax
   mov		rax, 19
   push		rax
   pop		rax
   mov		DWORD -528[rbp], eax
   mov		rax, 20
   push		rax
   pop		rax
   mov		DWORD -524[rbp], eax
   lea		rax, -536[rbp]
   push		rax
   pop		rax
   mov		-544[rbp], rax
   pop		rax
   mov		-552[rbp], rax
   pop		rax
   mov		-560[rbp], rax
   pop		rax
   mov		-568[rbp], rax
   pop		rax
   mov		-576[rbp], rax
   pop		rax
   mov		-584[rbp], rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   mov		rax, -584[rbp]
   mov		32[rsp], rax
   mov		rax, -576[rbp]
   mov		40[rsp], rax
   mov		rax, -568[rbp]
   mov		48[rsp], rax
   mov		rax, -560[rbp]
   mov		56[rsp], rax
   mov		rax, -552[rbp]
   mov		64[rsp], rax
   mov		rax, -544[rbp]
   mov		72[rsp], rax
   call		many_large_parameters
   lea		rbx, -456[rbp]
   push		rbx
   lea		rbx, -440[rbp]
   push		rbx
   lea		rbx, -424[rbp]
   push		rbx
   lea		rbx, -408[rbp]
   push		rbx
   lea		rbx, -392[rbp]
   push		rbx
   pop		rax
   lea		rbx, -72[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 35: $b : Vector4 = -88[rbp]
   pop		rax
   lea		rbx, -88[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 35: $c : Vector4 = -104[rbp]
   pop		rax
   lea		rbx, -104[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 35: $d : Vector4 = -120[rbp]
   pop		rax
   lea		rbx, -120[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 35: $e : Vector4 = -136[rbp]
   pop		rax
   lea		rbx, -136[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 36 Print
   lea		rax, -72[rbp]
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
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 12[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rax, -88[rbp]
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
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 12[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rax, -104[rbp]
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
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 12[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rax, -120[rbp]
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
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 12[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rax, -136[rbp]
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
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 12[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   ; Pop print arguments
   pop		rax
   mov		QWORD -384[rbp], rax
   pop		rax
   mov		QWORD -392[rbp], rax
   pop		rax
   mov		QWORD -400[rbp], rax
   pop		rax
   mov		QWORD -408[rbp], rax
   pop		rax
   mov		QWORD -416[rbp], rax
   pop		rax
   mov		QWORD -424[rbp], rax
   pop		rax
   mov		QWORD -432[rbp], rax
   pop		rax
   mov		QWORD -440[rbp], rax
   pop		rax
   mov		QWORD -448[rbp], rax
   pop		rax
   mov		QWORD -456[rbp], rax
   pop		rax
   mov		QWORD -464[rbp], rax
   pop		rax
   mov		QWORD -472[rbp], rax
   pop		rax
   mov		QWORD -480[rbp], rax
   pop		rax
   mov		QWORD -488[rbp], rax
   pop		rax
   mov		QWORD -496[rbp], rax
   pop		rax
   mov		QWORD -504[rbp], rax
   pop		rax
   mov		QWORD -512[rbp], rax
   pop		rax
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS4
   mov		rax, -512[rbp]
   mov		QWORD 32[rsp], rax
   mov		rax, -504[rbp]
   mov		QWORD 40[rsp], rax
   mov		rax, -496[rbp]
   mov		QWORD 48[rsp], rax
   mov		rax, -488[rbp]
   mov		QWORD 56[rsp], rax
   mov		rax, -480[rbp]
   mov		QWORD 64[rsp], rax
   mov		rax, -472[rbp]
   mov		QWORD 72[rsp], rax
   mov		rax, -464[rbp]
   mov		QWORD 80[rsp], rax
   mov		rax, -456[rbp]
   mov		QWORD 88[rsp], rax
   mov		rax, -448[rbp]
   mov		QWORD 96[rsp], rax
   mov		rax, -440[rbp]
   mov		QWORD 104[rsp], rax
   mov		rax, -432[rbp]
   mov		QWORD 112[rsp], rax
   mov		rax, -424[rbp]
   mov		QWORD 120[rsp], rax
   mov		rax, -416[rbp]
   mov		QWORD 128[rsp], rax
   mov		rax, -408[rbp]
   mov		QWORD 136[rsp], rax
   mov		rax, -400[rbp]
   mov		QWORD 144[rsp], rax
   mov		rax, -392[rbp]
   mov		QWORD 152[rsp], rax
   mov		rax, -384[rbp]
   mov		QWORD 160[rsp], rax
   call		printf
   ; Ln 38 Print
   ; Pop print arguments
   mov		rcx, CS5
   call		printf
   ; Ln 40: $a : u8 = -137[rbp]
   mov		rax, 254
   push		rax
   pop		rax
   mov		BYTE -137[rbp], al
   ; Ln 41: $b : f32 = -144[rbp]
   movss		xmm0, [CF6]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-144[rbp], eax
   ; Ln 42: $c : Vector4 = -160[rbp]
   lea		rcx, -160[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   mov		rax, 1
   push		rax
   pop		rax
   mov		DWORD -160[rbp], eax
   mov		rax, 2
   push		rax
   pop		rax
   mov		DWORD -156[rbp], eax
   mov		rax, 3
   push		rax
   pop		rax
   mov		DWORD -152[rbp], eax
   mov		rax, 4
   push		rax
   pop		rax
   mov		DWORD -148[rbp], eax
   ; Ln 43: $d : f64 = -168[rbp]
   movsd		xmm0, [CF7]
   movq		rax, xmm0
   push		rax
   pop		rax
   mov		-168[rbp], rax
   ; Ln 44: $e : bool = -169[rbp]
   push		1
   pop		rax
   mov		BYTE -169[rbp], al
   ; Ln 45: $f : Vector2 = -184[rbp]
   lea		rcx, -184[rbp]
   mov		rdx, 0
   mov		r8, 8
   call		memset
   movss		xmm0, [CF8]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-184[rbp], eax
   movss		xmm0, [CF9]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-180[rbp], eax
   ; Ln 46: $g : string = -192[rbp]
   mov		rax, CS10
   push		rax
   pop		rax
   mov		QWORD -192[rbp], rax
   ; Ln 47: $hv : Vector4 = -208[rbp]
   lea		rcx, -208[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   mov		rax, 1337
   push		rax
   pop		rax
   mov		DWORD -208[rbp], eax
   mov		rax, 7
   push		rax
   pop		rax
   mov		DWORD -204[rbp], eax
   mov		rax, 9
   push		rax
   pop		rax
   mov		DWORD -200[rbp], eax
   mov		rax, 13
   push		rax
   pop		rax
   mov		DWORD -196[rbp], eax
   ; Ln 48: $h : *Vector4 = -216[rbp]
   lea		rax, -208[rbp]
   push		rax
   pop		rax
   mov		QWORD -216[rbp], rax
   ; Ln 49: $i : s64 = -224[rbp]
   mov		rax, 5
   push		rax
   pop		rax
   neg		rax
   push		rax
   pop		rax
   mov		QWORD -224[rbp], rax
   ; Ln 51: $a_0 : u8 = -225[rbp]
   lea		rax, -377[rbp]
   push		rax
   lea		rax, -381[rbp]
   push		rax
   lea		rax, -397[rbp]
   push		rax
   lea		rax, -405[rbp]
   push		rax
   lea		rax, -406[rbp]
   push		rax
   lea		rax, -414[rbp]
   push		rax
   lea		rax, -422[rbp]
   push		rax
   lea		rax, -430[rbp]
   push		rax
   lea		rax, -438[rbp]
   push		rax
   movzx		eax, BYTE -137[rbp]
   push		rax
   mov		eax, -144[rbp]
   push		rax
   lea		rax, -160[rbp]
   push		rax
   mov		rax, -168[rbp]
   push		rax
   movzx		eax, BYTE -169[rbp]
   push		rax
   mov		rax, -184[rbp]
   push		rax
   mov		rax, QWORD -192[rbp]
   push		rax
   mov		rax, -216[rbp]
   push		rax
   mov		rax, QWORD -224[rbp]
   push		rax
   pop		rax
   mov		-446[rbp], rax
   pop		rax
   mov		-454[rbp], rax
   pop		rax
   mov		-462[rbp], rax
   pop		rax
   mov		-470[rbp], rax
   pop		rax
   mov		-478[rbp], rax
   pop		rax
   mov		-486[rbp], rax
   pop		rax
   mov		-494[rbp], rax
   pop		rax
   mov		-502[rbp], rax
   pop		rax
   mov		-510[rbp], rax
   pop		rax
   mov		-518[rbp], rax
   pop		rax
   mov		-526[rbp], rax
   pop		rax
   mov		-534[rbp], rax
   pop		rax
   mov		-542[rbp], rax
   pop		rax
   mov		-550[rbp], rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   mov		rax, -550[rbp]
   mov		32[rsp], rax
   mov		rax, -542[rbp]
   mov		40[rsp], rax
   mov		rax, -534[rbp]
   mov		48[rsp], rax
   mov		rax, -526[rbp]
   mov		56[rsp], rax
   mov		rax, -518[rbp]
   mov		64[rsp], rax
   mov		rax, -510[rbp]
   mov		72[rsp], rax
   mov		rax, -502[rbp]
   mov		80[rsp], rax
   mov		rax, -494[rbp]
   mov		88[rsp], rax
   mov		rax, -486[rbp]
   mov		96[rsp], rax
   mov		rax, -478[rbp]
   mov		104[rsp], rax
   mov		rax, -470[rbp]
   mov		112[rsp], rax
   mov		rax, -462[rbp]
   mov		120[rsp], rax
   mov		rax, -454[rbp]
   mov		128[rsp], rax
   mov		rax, -446[rbp]
   mov		136[rsp], rax
   call		many_mixed_parameters
   mov		rbx, -438[rbp]
   push		rbx
   mov		rbx, -430[rbp]
   push		rbx
   mov		rbx, -422[rbp]
   push		rbx
   mov		rbx, -414[rbp]
   push		rbx
   mov		rbx, -406[rbp]
   push		rbx
   mov		rbx, -405[rbp]
   push		rbx
   lea		rbx, -397[rbp]
   push		rbx
   mov		rbx, -381[rbp]
   push		rbx
   mov		rbx, -377[rbp]
   push		rbx
   pop		rax
   mov		BYTE -225[rbp], al
   ; Ln 51: $b_0 : f32 = -232[rbp]
   pop		rax
   mov		-232[rbp], eax
   ; Ln 51: $c_0 : Vector4 = -248[rbp]
   pop		rax
   lea		rbx, -248[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 51: $d_0 : f64 = -256[rbp]
   pop		rax
   mov		-256[rbp], rax
   ; Ln 51: $e_0 : bool = -257[rbp]
   pop		rax
   mov		BYTE -257[rbp], al
   ; Ln 51: $f_0 : Vector2 = -272[rbp]
   pop		rax
   mov		-272[rbp], rax
   ; Ln 51: $g_0 : string = -280[rbp]
   pop		rax
   mov		QWORD -280[rbp], rax
   ; Ln 51: $h_0 : *Vector4 = -288[rbp]
   pop		rax
   mov		QWORD -288[rbp], rax
   ; Ln 51: $i_0 : s64 = -296[rbp]
   pop		rax
   mov		QWORD -296[rbp], rax
   ; Ln 53 Print
   movzx		eax, BYTE -225[rbp]
   push		rax
   mov		eax, -232[rbp]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   lea		rax, -248[rbp]
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
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 12[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		rax, -256[rbp]
   push		rax
   movzx		eax, BYTE -257[rbp]
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
   mov		rax, -272[rbp]
   push		rax
   pop		rax
   mov		-384[rbp], rax
   lea		r9, -384[rbp]
   lea		rbx, 0[r9]
   mov		eax, [rbx]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   lea		rbx, 4[r9]
   mov		eax, [rbx]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   mov		rax, QWORD -280[rbp]
   push		rax
   mov		rax, -288[rbp]
   push		rax
   mov		rax, QWORD -296[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		QWORD -392[rbp], rax
   pop		rax
   mov		QWORD -400[rbp], rax
   pop		rax
   mov		QWORD -408[rbp], rax
   pop		rax
   mov		QWORD -416[rbp], rax
   pop		rax
   mov		QWORD -424[rbp], rax
   pop		rax
   mov		QWORD -432[rbp], rax
   pop		rax
   mov		QWORD -440[rbp], rax
   pop		rax
   mov		QWORD -448[rbp], rax
   pop		rax
   mov		QWORD -456[rbp], rax
   pop		rax
   mov		QWORD -464[rbp], rax
   pop		rax
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   movzx		rax, al
   mov		rdx, rax
   mov		rcx, CS11
   mov		rax, -464[rbp]
   mov		QWORD 32[rsp], rax
   mov		rax, -456[rbp]
   mov		QWORD 40[rsp], rax
   mov		rax, -448[rbp]
   mov		QWORD 48[rsp], rax
   mov		rax, -440[rbp]
   mov		QWORD 56[rsp], rax
   mov		rax, -432[rbp]
   mov		QWORD 64[rsp], rax
   mov		rax, -424[rbp]
   mov		QWORD 72[rsp], rax
   mov		rax, -416[rbp]
   mov		QWORD 80[rsp], rax
   mov		rax, -408[rbp]
   mov		QWORD 88[rsp], rax
   mov		rax, -400[rbp]
   mov		QWORD 96[rsp], rax
   mov		rax, -392[rbp]
   mov		QWORD 104[rsp], rax
   call		printf
   ; Ln 55 Print
   ; Pop print arguments
   mov		rcx, CS12
   call		printf
   ; Ln 57: $a : float = -300[rbp]
   lea		rax, -380[rbp]
   push		rax
   lea		rax, -384[rbp]
   push		rax
   movss		xmm0, [CF13]
   movd		eax, xmm0
   push		rax
   movss		xmm0, [CF14]
   movd		eax, xmm0
   push		rax
   pop		rax
   movd		xmm3, eax
   pop		rax
   movd		xmm2, eax
   pop		rdx
   pop		rcx
   call		float_parameters
   mov		rbx, -384[rbp]
   push		rbx
   mov		rbx, -380[rbp]
   push		rbx
   pop		rax
   mov		-300[rbp], eax
   ; Ln 57: $b : float = -304[rbp]
   pop		rax
   mov		-304[rbp], eax
   ; Ln 58 Print
   mov		eax, -300[rbp]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   mov		eax, -304[rbp]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   ; Pop print arguments
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS15
   call		printf
L6:
   mov		rax, 0
   add		rsp, 736
   pop		rbp
   ret
