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
   enum_to_int_buffer times 20 DB 0
   CS0 DB `The quick brown fox jumps over the lazy dog`, 0 
   CS1 DB `%s\n`, 0 
   CS2 DB `%lld\n`, 0 
   CS3 DB `%s\n`, 0 
   CS4 DB `%lld\n`, 0 
   CS5 DB `The quick brown fox jumps over the lazy dog`, 0 
   CS6 DB `%s\n`, 0 
   CS7 DB `%s`, 0 
   CS8 DB `The quick brown fox jumps over the lazy dog`, 0 
   CS9 DB `%s\n`, 0 
   CS10 DB `THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG`, 0 
   CS11 DB `%s\n`, 0 
   CS12 DB `The quick brown fox jumps over the lazy dog`, 0 
   CS13 DB `The`, 0 
   CS14 DB `The quick brown fox jumps over the lazy dog`, 0 
   CS15 DB `dog`, 0 
   CS16 DB `the quick brown fox jumps over the lazy dog`, 0 
   CS17 DB `quick`, 0 
   CS18 DB `dog`, 0 
   CS19 DB `quick`, 0 
   CS20 DB `the`, 0 
   CS21 DB `The quick brown fox jumps over the lazy dog`, 0 
   CS22 DB ` `, 0 
   CS23 DB `%s`, 0 
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
   extern Type_uint
   extern Type_u8
   extern Type_u16
   extern Type_u32
   extern Type_u64
   extern Type_int
   extern Type_s8
   extern Type_s16
   extern Type_s32
   extern Type_s64
   extern Type_float
   extern Type_f32
   extern Type_f64
   extern Type_string
   extern Type_bool
   extern Type_void
   extern Type_untyped_int
   extern Type_untyped_float
   extern memset
   extern memcpy
   extern malloc
   extern realloc
   extern free
   extern exit


assert:
   cmp		cl, 0
   jz 		assert_fail
   ret
assert_fail:
   mov		rcx, string_assert_fail
   call		printf
   mov		rcx, 1
   call		ExitProcess


; bytes locals   : 88
; bytes temp     : 0
; bytes total    : 128
; (ret_0: *string, s: string, from: int, to: int)
String.slice:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 128
   mov		16[rbp], rcx
   mov		24[rbp], rdx
   mov		32[rbp], r8d
   mov		40[rbp], r9d
   ; Param ret_0
   mov		rax, 16[rbp]
   mov		-8[rbp], rax
   ; Param s
   mov		rax, 24[rbp]
   lea		rbx, -24[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Param from
   mov		rax, 32[rbp]
   mov		-28[rbp], eax
   ; Param to
   mov		rax, 40[rbp]
   mov		-32[rbp], eax
   ; Ln 14: $result : string = -48[rbp]
   mov		QWORD -48[rbp], 0
   mov		QWORD -40[rbp], 0
   mov		eax, DWORD -32[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 1
   push		rax
   pop		rax
   neg		rax
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L2
   ; block of if
   ; Ln 17: Assignment
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   mov		eax, DWORD -28[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   lea		rax, -48[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   ; Ln 18: Assignment
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   mov		eax, DWORD -28[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   lea		rax, -48[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   jmp L1
; else
L2:
   ; Ln 20: Assignment
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   mov		eax, DWORD -28[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   lea		rax, -48[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   ; Ln 21: Assignment
   mov		eax, DWORD -32[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -28[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   lea		rax, -48[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   jmp L1
; done
L1:
   ; Return
   lea		rax, -48[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   jmp		L0
L0:
   mov		rax, 0
   add		rsp, 128
   pop		rbp
   ret

; bytes locals   : 56
; bytes temp     : 64
; bytes total    : 160
; (ret_0: *string, s: string)
String.copy:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 160
   mov		16[rbp], rcx
   mov		24[rbp], rdx
   ; Param ret_0
   mov		rax, 16[rbp]
   mov		-8[rbp], rax
   ; Param s
   mov		rax, 24[rbp]
   lea		rbx, -24[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 28: $result : string = -40[rbp]
   mov		QWORD -40[rbp], 0
   mov		QWORD -32[rbp], 0
   ; Ln 30: Assignment
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   pop		rcx
   ; (s64) 
   call		malloc
   push		rax
   lea		rax, -40[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   lea		rax, -40[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*char, *char, s64) 
   call		memcpy
   push		rax
   pop		rax
   ; Ln 33: Assignment
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   lea		rax, -40[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   ; Return
   lea		rax, -40[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   jmp		L3
L3:
   mov		rax, 0
   add		rsp, 160
   pop		rbp
   ret

; bytes locals   : 88
; bytes temp     : 48
; bytes total    : 176
; (ret_0: *string, s: string)
String.toUpper:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 176
   mov		16[rbp], rcx
   mov		24[rbp], rdx
   ; Param ret_0
   mov		rax, 16[rbp]
   mov		-8[rbp], rax
   ; Param s
   mov		rax, 24[rbp]
   lea		rbx, -24[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 39: $result : string = -40[rbp]
   lea		rax, -104[rbp]
   push		rax
   lea		rax, -24[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (string) 
   call		String.copy
   lea		rax, -104[rbp]
   push		rax
   pop		rax
   lea		rbx, -40[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 41: For-loop
   lea		rax, -24[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-56[rbp], rbx     ; data
   mov		-64[rbp], rcx     ; count
   mov		QWORD -72[rbp], 0 ; index
L5:
   mov		rbx, -64[rbp]
   mov		rax, -72[rbp]
   cmp		rax, rbx
   jge		L7
   mov		rbx, QWORD -56[rbp]
   mov		rax, QWORD -72[rbp]
   imul		rax, 1
   lea		rbx, [rbx + rax]
   mov		al, BYTE [rbx]
   mov		-41[rbp], al 
   movzx		eax, BYTE -41[rbp]
   push		rax
   mov		rax, 97
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setge		al
   push		rax
   movzx		eax, BYTE -41[rbp]
   push		rax
   mov		rax, 122
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setle		al
   push		rax

   pop		rbx
   pop		rax
   and		al, bl
   push		rax
   pop		rax
   cmp		al, 0
   jz			L8
   ; block of if
   ; Ln 43: Assignment
   movzx		eax, BYTE -41[rbp]
   push		rax
   mov		rax, 32
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   lea		rax, -40[rbp]
   push		rax
   mov		eax, DWORD -72[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   mov		rbx, [rbx]
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   pop		rbx
   pop		rax
   mov		BYTE [rbx], al
   jmp L8
; done
L8:
L6:
   inc		QWORD -72[rbp]
   jmp		L5
L7:
   ; Return
   lea		rax, -40[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   jmp		L4
L4:
   mov		rax, 0
   add		rsp, 176
   pop		rbp
   ret

; bytes locals   : 88
; bytes temp     : 48
; bytes total    : 176
; (ret_0: *string, s: string)
String.toLower:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 176
   mov		16[rbp], rcx
   mov		24[rbp], rdx
   ; Param ret_0
   mov		rax, 16[rbp]
   mov		-8[rbp], rax
   ; Param s
   mov		rax, 24[rbp]
   lea		rbx, -24[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 51: $result : string = -40[rbp]
   lea		rax, -104[rbp]
   push		rax
   lea		rax, -24[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (string) 
   call		String.copy
   lea		rax, -104[rbp]
   push		rax
   pop		rax
   lea		rbx, -40[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 53: For-loop
   lea		rax, -24[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-56[rbp], rbx     ; data
   mov		-64[rbp], rcx     ; count
   mov		QWORD -72[rbp], 0 ; index
L10:
   mov		rbx, -64[rbp]
   mov		rax, -72[rbp]
   cmp		rax, rbx
   jge		L12
   mov		rbx, QWORD -56[rbp]
   mov		rax, QWORD -72[rbp]
   imul		rax, 1
   lea		rbx, [rbx + rax]
   mov		al, BYTE [rbx]
   mov		-41[rbp], al 
   movzx		eax, BYTE -41[rbp]
   push		rax
   mov		rax, 65
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setge		al
   push		rax
   movzx		eax, BYTE -41[rbp]
   push		rax
   mov		rax, 90
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setle		al
   push		rax

   pop		rbx
   pop		rax
   and		al, bl
   push		rax
   pop		rax
   cmp		al, 0
   jz			L13
   ; block of if
   ; Ln 55: Assignment
   movzx		eax, BYTE -41[rbp]
   push		rax
   mov		rax, 32
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   lea		rax, -40[rbp]
   push		rax
   mov		eax, DWORD -72[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   mov		rbx, [rbx]
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   pop		rbx
   pop		rax
   mov		BYTE [rbx], al
   jmp L13
; done
L13:
L11:
   inc		QWORD -72[rbp]
   jmp		L10
L12:
   ; Return
   lea		rax, -40[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   jmp		L9
L9:
   mov		rax, 0
   add		rsp, 176
   pop		rbp
   ret

; bytes locals   : 104
; bytes temp     : 0
; bytes total    : 144
; (ret_0: *bool, s: string, str: string)
String.startsWith:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 144
   mov		16[rbp], rcx
   mov		24[rbp], rdx
   mov		32[rbp], r8 
   ; Param ret_0
   mov		rax, 16[rbp]
   mov		-8[rbp], rax
   ; Param s
   mov		rax, 24[rbp]
   lea		rbx, -24[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Param str
   mov		rax, 32[rbp]
   lea		rbx, -40[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 63: For-loop
   lea		rax, -40[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-56[rbp], rbx     ; data
   mov		-64[rbp], rcx     ; count
   mov		QWORD -72[rbp], 0 ; index
L15:
   mov		rbx, -64[rbp]
   mov		rax, -72[rbp]
   cmp		rax, rbx
   jge		L17
   mov		rbx, QWORD -56[rbp]
   mov		rax, QWORD -72[rbp]
   imul		rax, 1
   lea		rbx, [rbx + rax]
   mov		al, BYTE [rbx]
   mov		-41[rbp], al 
   lea		rax, -24[rbp]
   push		rax
   mov		eax, DWORD -72[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   mov		rbx, [rbx]
   imul		rax, 1; Add index expr
   add		rbx, rax
   movzx		eax, BYTE [rbx]
   push		rax
   movzx		eax, BYTE -41[rbp]
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setne		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L18
   ; block of if
   ; Return
   push		0
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L14
   jmp L18
; done
L18:
L16:
   inc		QWORD -72[rbp]
   jmp		L15
L17:
   ; Return
   push		1
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L14
L14:
   mov		rax, 0
   add		rsp, 144
   pop		rbp
   ret

; bytes locals   : 120
; bytes temp     : 80
; bytes total    : 240
; (ret_0: *bool, s: string, str: string)
String.endsWith:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 240
   mov		16[rbp], rcx
   mov		24[rbp], rdx
   mov		32[rbp], r8 
   ; Param ret_0
   mov		rax, 16[rbp]
   mov		-8[rbp], rax
   ; Param s
   mov		rax, 24[rbp]
   lea		rbx, -24[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Param str
   mov		rax, 32[rbp]
   lea		rbx, -40[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   lea		rax, -40[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setl		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L20
   ; block of if
   ; Return
   push		0
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L19
   jmp L20
; done
L20:
   ; Ln 77: $trailing : string = -56[rbp]
   lea		rax, -136[rbp]
   push		rax
   lea		rax, -24[rbp]
   push		rax
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   lea		rax, -40[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   mov		rax, 1
   push		rax
   pop		rax
   neg		rax
   push		rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, s64, int) 
   call		String.slice
   lea		rax, -136[rbp]
   push		rax
   pop		rax
   lea		rbx, -56[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 79: For-loop
   lea		rax, -56[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-72[rbp], rbx     ; data
   mov		-80[rbp], rcx     ; count
   mov		QWORD -88[rbp], 0 ; index
L21:
   mov		rbx, -80[rbp]
   mov		rax, -88[rbp]
   cmp		rax, rbx
   jge		L23
   mov		rbx, QWORD -72[rbp]
   mov		rax, QWORD -88[rbp]
   imul		rax, 1
   lea		rbx, [rbx + rax]
   mov		al, BYTE [rbx]
   mov		-57[rbp], al 
   lea		rax, -40[rbp]
   push		rax
   mov		eax, DWORD -88[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   mov		rbx, [rbx]
   imul		rax, 1; Add index expr
   add		rbx, rax
   movzx		eax, BYTE [rbx]
   push		rax
   movzx		eax, BYTE -57[rbp]
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setne		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L24
   ; block of if
   ; Return
   push		0
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L19
   jmp L24
; done
L24:
L22:
   inc		QWORD -88[rbp]
   jmp		L21
L23:
   ; Return
   push		1
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L19
L19:
   mov		rax, 0
   add		rsp, 240
   pop		rbp
   ret

; bytes locals   : 104
; bytes temp     : 0
; bytes total    : 144
; (ret_0: *bool, s: string, str: string)
String.equals:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 144
   mov		16[rbp], rcx
   mov		24[rbp], rdx
   mov		32[rbp], r8 
   ; Param ret_0
   mov		rax, 16[rbp]
   mov		-8[rbp], rax
   ; Param s
   mov		rax, 24[rbp]
   lea		rbx, -24[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Param str
   mov		rax, 32[rbp]
   lea		rbx, -40[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   lea		rax, -40[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setne		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L26
   ; block of if
   ; Return
   push		0
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L25
   jmp L26
; done
L26:
   ; Ln 94: For-loop
   lea		rax, -24[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-56[rbp], rbx     ; data
   mov		-64[rbp], rcx     ; count
   mov		QWORD -72[rbp], 0 ; index
L27:
   mov		rbx, -64[rbp]
   mov		rax, -72[rbp]
   cmp		rax, rbx
   jge		L29
   mov		rbx, QWORD -56[rbp]
   mov		rax, QWORD -72[rbp]
   imul		rax, 1
   lea		rbx, [rbx + rax]
   mov		al, BYTE [rbx]
   mov		-41[rbp], al 
   lea		rax, -40[rbp]
   push		rax
   mov		eax, DWORD -72[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   mov		rbx, [rbx]
   imul		rax, 1; Add index expr
   add		rbx, rax
   movzx		eax, BYTE [rbx]
   push		rax
   movzx		eax, BYTE -41[rbp]
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setne		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L30
   ; block of if
   ; Return
   push		0
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L25
   jmp L30
; done
L30:
L28:
   inc		QWORD -72[rbp]
   jmp		L27
L29:
   ; Return
   push		1
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L25
L25:
   mov		rax, 0
   add		rsp, 144
   pop		rbp
   ret

; bytes locals   : 184
; bytes temp     : 72
; bytes total    : 288
; (ret_0: *int, s: string, str: string, fromIndex: int)
String.indexOf:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 288
   mov		16[rbp], rcx
   mov		24[rbp], rdx
   mov		32[rbp], r8 
   mov		40[rbp], r9d
   ; Param ret_0
   mov		rax, 16[rbp]
   mov		-8[rbp], rax
   ; Param s
   mov		rax, 24[rbp]
   lea		rbx, -24[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Param str
   mov		rax, 32[rbp]
   lea		rbx, -40[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Param fromIndex
   mov		rax, 40[rbp]
   mov		-44[rbp], eax
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   mov		rax, 0
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   lea		rax, -40[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   mov		rax, 0
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax

   pop		rbx
   pop		rax
   or 		al, bl
   push		rax
   pop		rax
   cmp		al, 0
   jz			L32
   ; block of if
   ; Return
   mov		rax, 1
   push		rax
   pop		rax
   neg		rax
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   mov		DWORD [rbx], eax
   jmp		L31
   jmp L32
; done
L32:
   mov		eax, DWORD -44[rbp]
   movsx		rax, eax
   push		rax
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setge		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L33
   ; block of if
   ; Return
   mov		rax, 1
   push		rax
   pop		rax
   neg		rax
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   mov		DWORD [rbx], eax
   jmp		L31
   jmp L33
; done
L33:
   ; Ln 113: $src : string = -64[rbp]
   lea		rax, -200[rbp]
   push		rax
   lea		rax, -24[rbp]
   push		rax
   mov		eax, DWORD -44[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 1
   push		rax
   pop		rax
   neg		rax
   push		rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, int, int) 
   call		String.slice
   lea		rax, -200[rbp]
   push		rax
   pop		rax
   lea		rbx, -64[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 115: For-loop
   lea		rax, -64[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-80[rbp], rbx     ; data
   mov		-88[rbp], rcx     ; count
   mov		QWORD -96[rbp], 0 ; index
L34:
   mov		rbx, -88[rbp]
   mov		rax, -96[rbp]
   cmp		rax, rbx
   jge		L36
   mov		rbx, QWORD -80[rbp]
   mov		rax, QWORD -96[rbp]
   imul		rax, 1
   lea		rbx, [rbx + rax]
   mov		al, BYTE [rbx]
   mov		-65[rbp], al 
   lea		rax, -64[rbp]
   push		rax
   mov		eax, DWORD -96[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   mov		rbx, [rbx]
   imul		rax, 1; Add index expr
   add		rbx, rax
   movzx		eax, BYTE [rbx]
   push		rax
   lea		rax, -40[rbp]
   push		rax
   mov		rax, 0
   push		rax
   pop		rax
   pop		rbx
   mov		rbx, [rbx]
   imul		rax, 1; Add index expr
   add		rbx, rax
   movzx		eax, BYTE [rbx]
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L37
   ; block of if
   ; Ln 119: $match : bool = -97[rbp]
   push		1
   pop		rax
   mov		BYTE -97[rbp], al
   ; Ln 120: For-loop
   lea		rax, -40[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-112[rbp], rbx     ; data
   mov		-120[rbp], rcx     ; count
   mov		QWORD -128[rbp], 0 ; index
L38:
   mov		rbx, -120[rbp]
   mov		rax, -128[rbp]
   cmp		rax, rbx
   jge		L40
   mov		rbx, QWORD -112[rbp]
   mov		rax, QWORD -128[rbp]
   imul		rax, 1
   lea		rbx, [rbx + rax]
   mov		al, BYTE [rbx]
   mov		-98[rbp], al 
   mov		eax, DWORD -96[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -128[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   lea		rax, -64[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setge		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L41
   ; block of if
   ; Return
   mov		rax, 1
   push		rax
   pop		rax
   neg		rax
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   mov		DWORD [rbx], eax
   jmp		L31
   jmp L41
; done
L41:
   ; Ln 127: $c1f : char = -129[rbp]
   lea		rax, -64[rbp]
   push		rax
   mov		eax, DWORD -96[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -128[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   pop		rax
   pop		rbx
   mov		rbx, [rbx]
   imul		rax, 1; Add index expr
   add		rbx, rax
   movzx		eax, BYTE [rbx]
   push		rax
   pop		rax
   mov		BYTE -129[rbp], al
   movzx		eax, BYTE -98[rbp]
   push		rax
   movzx		eax, BYTE -129[rbp]
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setne		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L42
   ; block of if
   ; Ln 131: Assignment
   push		0
   lea		rax, -97[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		BYTE [rbx], al
   jmp		L40
   jmp L42
; done
L42:
L39:
   inc		QWORD -128[rbp]
   jmp		L38
L40:
   movzx		eax, BYTE -97[rbp]
   push		rax
   pop		rax
   cmp		al, 0
   jz			L43
   ; block of if
   ; Return
   mov		eax, DWORD -96[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   mov		DWORD [rbx], eax
   jmp		L31
   jmp L43
; done
L43:
   jmp L37
; done
L37:
L35:
   inc		QWORD -96[rbp]
   jmp		L34
L36:
   ; Return
   mov		rax, 1
   push		rax
   pop		rax
   neg		rax
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   mov		DWORD [rbx], eax
   jmp		L31
L31:
   mov		rax, 0
   add		rsp, 288
   pop		rbp
   ret

; bytes locals   : 96
; bytes temp     : 0
; bytes total    : 128
; (ret_0: *[]string, s: string, sep: string)
String.split:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 128
   mov		16[rbp], rcx
   mov		24[rbp], rdx
   mov		32[rbp], r8 
   ; Param ret_0
   mov		rax, 16[rbp]
   mov		-8[rbp], rax
   ; Param s
   mov		rax, 24[rbp]
   lea		rbx, -24[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Param sep
   mov		rax, 32[rbp]
   lea		rbx, -40[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 146: $result : [..]string = -64[rbp]
   lea		rbx, -64[rbp]
   push		rbx
   mov		rdx, 16
   mov		rcx, 2
   call		calloc
   pop		rbx
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 0
   mov		QWORD 16[rbx], 2
   ; Return
   lea		rax, -64[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   jmp		L44
L44:
   mov		rax, 0
   add		rsp, 128
   pop		rbp
   ret

; bytes locals   : 296
; bytes temp     : 72
; bytes total    : 400
; ()
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 400
   push		0
   pop		rax
   cmp		al, 0
   jz			L46
   ; block of if
   ; Ln 4: $text : string = -16[rbp]
   mov		rax, CS0
   mov		-312[rbp], rax
   mov		QWORD -304[rbp], 43
   lea		rax, -312[rbp]
   push		rax
   pop		rax
   lea		rbx, -16[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 6: $quickBrown : string = -32[rbp]
   lea		rax, -312[rbp]
   push		rax
   lea		rax, -16[rbp]
   push		rax
   mov		rax, 4
   push		rax
   mov		rax, 15
   push		rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, int, int) 
   call		String.slice
   lea		rax, -312[rbp]
   push		rax
   pop		rax
   lea		rbx, -32[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 8 Print
   lea		rax, -32[rbp]
   push		rax
   pop		rbx
   mov		-304[rbp], rbx
   mov		rdx, 8[rbx]
   add		rdx, 1
   mov		rcx, 1
   sub		rsp, 32
   call		calloc
   add		rsp, 32
   mov		rbx, -304[rbp]
   mov		rdx, 0[rbx]
   mov		r8d, 8[rbx]
   mov		-304[rbp], rax
   mov		rcx, rax
   sub		rsp, 32
   call		memcpy
   add		rsp, 32
   mov		rax, -304[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS1
   call		printf
   ; Ln 9 Print
   lea		rax, -32[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS2
   call		printf
   ; Ln 11: $fox : string = -48[rbp]
   lea		rax, -312[rbp]
   push		rax
   lea		rax, -16[rbp]
   push		rax
   mov		rax, 16
   push		rax
   mov		rax, 1
   push		rax
   pop		rax
   neg		rax
   push		rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, int, int) 
   call		String.slice
   lea		rax, -312[rbp]
   push		rax
   pop		rax
   lea		rbx, -48[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 12 Print
   lea		rax, -48[rbp]
   push		rax
   pop		rbx
   mov		-304[rbp], rbx
   mov		rdx, 8[rbx]
   add		rdx, 1
   mov		rcx, 1
   sub		rsp, 32
   call		calloc
   add		rsp, 32
   mov		rbx, -304[rbp]
   mov		rdx, 0[rbx]
   mov		r8d, 8[rbx]
   mov		-304[rbp], rax
   mov		rcx, rax
   sub		rsp, 32
   call		memcpy
   add		rsp, 32
   mov		rax, -304[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS3
   call		printf
   ; Ln 13 Print
   lea		rax, -48[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS4
   call		printf
   jmp L46
; done
L46:
   push		0
   pop		rax
   cmp		al, 0
   jz			L47
   ; block of if
   ; Ln 17: $text : string = -64[rbp]
   mov		rax, CS5
   mov		-312[rbp], rax
   mov		QWORD -304[rbp], 43
   lea		rax, -312[rbp]
   push		rax
   pop		rax
   lea		rbx, -64[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 19: $copy : string = -80[rbp]
   lea		rax, -312[rbp]
   push		rax
   lea		rax, -64[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (string) 
   call		String.copy
   lea		rax, -312[rbp]
   push		rax
   pop		rax
   lea		rbx, -80[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 21: Assignment
   mov		rax, 115
   push		rax
   lea		rax, -64[rbp]
   push		rax
   mov		rax, 4
   push		rax
   pop		rax
   pop		rbx
   mov		rbx, [rbx]
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   pop		rbx
   pop		rax
   mov		BYTE [rbx], al
   ; Ln 22: Assignment
   mov		rax, 108
   push		rax
   lea		rax, -64[rbp]
   push		rax
   mov		rax, 5
   push		rax
   pop		rax
   pop		rbx
   mov		rbx, [rbx]
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   pop		rbx
   pop		rax
   mov		BYTE [rbx], al
   ; Ln 23: Assignment
   mov		rax, 111
   push		rax
   lea		rax, -64[rbp]
   push		rax
   mov		rax, 6
   push		rax
   pop		rax
   pop		rbx
   mov		rbx, [rbx]
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   pop		rbx
   pop		rax
   mov		BYTE [rbx], al
   ; Ln 24: Assignment
   mov		rax, 119
   push		rax
   lea		rax, -64[rbp]
   push		rax
   mov		rax, 7
   push		rax
   pop		rax
   pop		rbx
   mov		rbx, [rbx]
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   pop		rbx
   pop		rax
   mov		BYTE [rbx], al
   ; Ln 25: Assignment
   mov		rax, 32
   push		rax
   lea		rax, -64[rbp]
   push		rax
   mov		rax, 8
   push		rax
   pop		rax
   pop		rbx
   mov		rbx, [rbx]
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   pop		rbx
   pop		rax
   mov		BYTE [rbx], al
   ; Ln 26 Print
   lea		rax, -64[rbp]
   push		rax
   pop		rbx
   mov		-304[rbp], rbx
   mov		rdx, 8[rbx]
   add		rdx, 1
   mov		rcx, 1
   sub		rsp, 32
   call		calloc
   add		rsp, 32
   mov		rbx, -304[rbp]
   mov		rdx, 0[rbx]
   mov		r8d, 8[rbx]
   mov		-304[rbp], rax
   mov		rcx, rax
   sub		rsp, 32
   call		memcpy
   add		rsp, 32
   mov		rax, -304[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS6
   call		printf
   ; Ln 28 Print
   lea		rax, -80[rbp]
   push		rax
   pop		rbx
   mov		-304[rbp], rbx
   mov		rdx, 8[rbx]
   add		rdx, 1
   mov		rcx, 1
   sub		rsp, 32
   call		calloc
   add		rsp, 32
   mov		rbx, -304[rbp]
   mov		rdx, 0[rbx]
   mov		r8d, 8[rbx]
   mov		-304[rbp], rax
   mov		rcx, rax
   sub		rsp, 32
   call		memcpy
   add		rsp, 32
   mov		rax, -304[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS7
   call		printf
   jmp L47
; done
L47:
   push		0
   pop		rax
   cmp		al, 0
   jz			L48
   ; block of if
   ; Ln 32: $text : string = -96[rbp]
   mov		rax, CS8
   mov		-312[rbp], rax
   mov		QWORD -304[rbp], 43
   lea		rax, -312[rbp]
   push		rax
   pop		rax
   lea		rbx, -96[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 34: $uppercased : string = -112[rbp]
   lea		rax, -312[rbp]
   push		rax
   lea		rax, -96[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (string) 
   call		String.toUpper
   lea		rax, -312[rbp]
   push		rax
   pop		rax
   lea		rbx, -112[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 35 Print
   lea		rax, -112[rbp]
   push		rax
   pop		rbx
   mov		-304[rbp], rbx
   mov		rdx, 8[rbx]
   add		rdx, 1
   mov		rcx, 1
   sub		rsp, 32
   call		calloc
   add		rsp, 32
   mov		rbx, -304[rbp]
   mov		rdx, 0[rbx]
   mov		r8d, 8[rbx]
   mov		-304[rbp], rax
   mov		rcx, rax
   sub		rsp, 32
   call		memcpy
   add		rsp, 32
   mov		rax, -304[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS9
   call		printf
   jmp L48
; done
L48:
   push		0
   pop		rax
   cmp		al, 0
   jz			L49
   ; block of if
   ; Ln 40: $text : string = -128[rbp]
   mov		rax, CS10
   mov		-312[rbp], rax
   mov		QWORD -304[rbp], 43
   lea		rax, -312[rbp]
   push		rax
   pop		rax
   lea		rbx, -128[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 42: $lowercase : string = -144[rbp]
   lea		rax, -312[rbp]
   push		rax
   lea		rax, -128[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (string) 
   call		String.toLower
   lea		rax, -312[rbp]
   push		rax
   pop		rax
   lea		rbx, -144[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 43 Print
   lea		rax, -144[rbp]
   push		rax
   pop		rbx
   mov		-304[rbp], rbx
   mov		rdx, 8[rbx]
   add		rdx, 1
   mov		rcx, 1
   sub		rsp, 32
   call		calloc
   add		rsp, 32
   mov		rbx, -304[rbp]
   mov		rdx, 0[rbx]
   mov		r8d, 8[rbx]
   mov		-304[rbp], rax
   mov		rcx, rax
   sub		rsp, 32
   call		memcpy
   add		rsp, 32
   mov		rax, -304[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS11
   call		printf
   jmp L49
; done
L49:
   push		1
   pop		rax
   cmp		al, 0
   jz			L50
   ; block of if
   ; Ln 48: $text : string = -160[rbp]
   mov		rax, CS12
   mov		-312[rbp], rax
   mov		QWORD -304[rbp], 43
   lea		rax, -312[rbp]
   push		rax
   pop		rax
   lea		rbx, -160[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -297[rbp]
   push		rax
   lea		rax, -160[rbp]
   push		rax
   mov		rax, CS13
   mov		-313[rbp], rax
   mov		QWORD -305[rbp], 3
   lea		rax, -313[rbp]
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, string) 
   call		String.startsWith
   movzx		eax, BYTE -297[rbp]
   push		rax
   pop		rcx
   mov		rdx, 49
   call		assert
   jmp L50
; done
L50:
   push		1
   pop		rax
   cmp		al, 0
   jz			L51
   ; block of if
   ; Ln 54: $text : string = -176[rbp]
   mov		rax, CS14
   mov		-312[rbp], rax
   mov		QWORD -304[rbp], 43
   lea		rax, -312[rbp]
   push		rax
   pop		rax
   lea		rbx, -176[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -297[rbp]
   push		rax
   lea		rax, -176[rbp]
   push		rax
   mov		rax, CS15
   mov		-313[rbp], rax
   mov		QWORD -305[rbp], 3
   lea		rax, -313[rbp]
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, string) 
   call		String.endsWith
   movzx		eax, BYTE -297[rbp]
   push		rax
   pop		rcx
   mov		rdx, 55
   call		assert
   jmp L51
; done
L51:
   push		1
   pop		rax
   cmp		al, 0
   jz			L52
   ; block of if
   ; Ln 60: $text : string = -192[rbp]
   mov		rax, CS16
   mov		-312[rbp], rax
   mov		QWORD -304[rbp], 43
   lea		rax, -312[rbp]
   push		rax
   pop		rax
   lea		rbx, -192[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 62: $index1 : int = -196[rbp]
   lea		rax, -300[rbp]
   push		rax
   lea		rax, -192[rbp]
   push		rax
   mov		rax, CS17
   mov		-316[rbp], rax
   mov		QWORD -308[rbp], 5
   lea		rax, -316[rbp]
   push		rax
   mov		rax, 0
   push		rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, string, int) 
   call		String.indexOf
   mov		eax, DWORD -300[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -196[rbp], eax
   mov		eax, DWORD -196[rbp]
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
   mov		rdx, 63
   call		assert
   ; Ln 65: $index2 : int = -200[rbp]
   lea		rax, -300[rbp]
   push		rax
   lea		rax, -192[rbp]
   push		rax
   mov		rax, CS18
   mov		-316[rbp], rax
   mov		QWORD -308[rbp], 3
   lea		rax, -316[rbp]
   push		rax
   mov		rax, 0
   push		rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, string, int) 
   call		String.indexOf
   mov		eax, DWORD -300[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -200[rbp], eax
   mov		eax, DWORD -200[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 40
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 66
   call		assert
   ; Ln 69: $index3 : int = -204[rbp]
   lea		rax, -300[rbp]
   push		rax
   lea		rax, -192[rbp]
   push		rax
   mov		rax, CS19
   mov		-316[rbp], rax
   mov		QWORD -308[rbp], 5
   lea		rax, -316[rbp]
   push		rax
   mov		rax, 10
   push		rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, string, int) 
   call		String.indexOf
   mov		eax, DWORD -300[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -204[rbp], eax
   mov		eax, DWORD -204[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 1
   push		rax
   pop		rax
   neg		rax
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 70
   call		assert
   ; Ln 72: $index4 : int = -208[rbp]
   lea		rax, -300[rbp]
   push		rax
   lea		rax, -192[rbp]
   push		rax
   mov		rax, CS20
   mov		-316[rbp], rax
   mov		QWORD -308[rbp], 3
   lea		rax, -316[rbp]
   push		rax
   mov		rax, 4
   push		rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, string, int) 
   call		String.indexOf
   mov		eax, DWORD -300[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -208[rbp], eax
   mov		eax, DWORD -208[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 27
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 73
   call		assert
   jmp L52
; done
L52:
   push		1
   pop		rax
   cmp		al, 0
   jz			L53
   ; block of if
   ; Ln 78: $text : string = -224[rbp]
   mov		rax, CS21
   mov		-312[rbp], rax
   mov		QWORD -304[rbp], 43
   lea		rax, -312[rbp]
   push		rax
   pop		rax
   lea		rbx, -224[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 80: $splitted : []string = -240[rbp]
   lea		rax, -312[rbp]
   push		rax
   lea		rax, -224[rbp]
   push		rax
   mov		rax, CS22
   mov		-328[rbp], rax
   mov		QWORD -320[rbp], 1
   lea		rax, -328[rbp]
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, string) 
   call		String.split
   lea		rax, -312[rbp]
   push		rax
   pop		rax
   lea		rbx, -240[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 82: For-loop
   lea		rax, -240[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-264[rbp], rbx     ; data
   mov		-272[rbp], rcx     ; count
   mov		QWORD -280[rbp], 0 ; index
L54:
   mov		rbx, -272[rbp]
   mov		rax, -280[rbp]
   cmp		rax, rbx
   jge		L56
   mov		rbx, QWORD -264[rbp]
   mov		rax, QWORD -280[rbp]
   imul		rax, 16
   lea		rbx, [rbx + rax]
   mov		r8, 16
   mov		rdx, rbx
   lea		rcx, -256[rbp]
   call		memcpy
   ; Ln 83 Print
   lea		rax, -256[rbp]
   push		rax
   pop		rbx
   mov		-304[rbp], rbx
   mov		rdx, 8[rbx]
   add		rdx, 1
   mov		rcx, 1
   sub		rsp, 32
   call		calloc
   add		rsp, 32
   mov		rbx, -304[rbp]
   mov		rdx, 0[rbx]
   mov		r8d, 8[rbx]
   mov		-304[rbp], rax
   mov		rcx, rax
   sub		rsp, 32
   call		memcpy
   add		rsp, 32
   mov		rax, -304[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS23
   call		printf
L55:
   inc		QWORD -280[rbp]
   jmp		L54
L56:
   jmp L53
; done
L53:
L45:
   mov		rax, 0
   add		rsp, 400
   pop		rbp
   ret
