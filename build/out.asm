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
   __TypeKind.Void DB "Void", 0
   __TypeKind.Bool DB "Bool", 0
   __TypeKind.Int DB "Int", 0
   __TypeKind.Float DB "Float", 0
   __TypeKind.String DB "String", 0
   __TypeKind.Array DB "Array", 0
   __TypeKind.Pointer DB "Pointer", 0
   __TypeKind.Struct DB "Struct", 0
   __TypeKind.Enum DB "Enum", 0
   __TypeKind.Function DB "Function", 0
   __TypeKind.Tuple DB "Tuple", 0
   __TypeKind.Variadic DB "Variadic", 0
   __TypeKind.Any DB "Any", 0
   __TypePrimitiveKind.Uint DB "Uint", 0
   __TypePrimitiveKind.U8 DB "U8", 0
   __TypePrimitiveKind.U16 DB "U16", 0
   __TypePrimitiveKind.U32 DB "U32", 0
   __TypePrimitiveKind.U64 DB "U64", 0
   __TypePrimitiveKind.Int DB "Int", 0
   __TypePrimitiveKind.S8 DB "S8", 0
   __TypePrimitiveKind.S16 DB "S16", 0
   __TypePrimitiveKind.S32 DB "S32", 0
   __TypePrimitiveKind.S64 DB "S64", 0
   __TypePrimitiveKind.Float DB "Float", 0
   __TypePrimitiveKind.F32 DB "F32", 0
   __TypePrimitiveKind.F64 DB "F64", 0
   __TypePrimitiveKind.String DB "String", 0
   __TypePrimitiveKind.Bool DB "Bool", 0
   __TypePrimitiveKind.Void DB "Void", 0
   __TypePrimitiveKind.UntypedInt DB "UntypedInt", 0
   __TypePrimitiveKind.UntypedFloat DB "UntypedFloat", 0
   __TypeArrayKind.Fixed DB "Fixed", 0
   __TypeArrayKind.Slice DB "Slice", 0
   __TypeArrayKind.Dynamic DB "Dynamic", 0
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
   CS23 DB `%s\n`, 0 
   CS24 DB `The quick brown fox jumps over the lazy dog`, 0 
   CS25 DB `god yzal eht revo spmuj xof nworb kciuq ehT`, 0 
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
   extern runtime_get_type_pointer
   extern runtime_get_type_array
   extern runtime_get_type_enum
   extern runtime_get_type_struct
   extern runtime_get_type_any
   extern runtime_builtin_append
   extern runtime_compare_strings
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

get_enum_string_TypeKind:
   mov		r8, 0
   cmp		rdx, r8
   jz			enum_case_0
   mov		r8, 1
   cmp		rdx, r8
   jz			enum_case_1
   mov		r8, 2
   cmp		rdx, r8
   jz			enum_case_2
   mov		r8, 3
   cmp		rdx, r8
   jz			enum_case_3
   mov		r8, 4
   cmp		rdx, r8
   jz			enum_case_4
   mov		r8, 5
   cmp		rdx, r8
   jz			enum_case_5
   mov		r8, 6
   cmp		rdx, r8
   jz			enum_case_6
   mov		r8, 7
   cmp		rdx, r8
   jz			enum_case_7
   mov		r8, 8
   cmp		rdx, r8
   jz			enum_case_8
   mov		r8, 9
   cmp		rdx, r8
   jz			enum_case_9
   mov		r8, 10
   cmp		rdx, r8
   jz			enum_case_10
   mov		r8, 11
   cmp		rdx, r8
   jz			enum_case_11
   mov		r8, 12
   cmp		rdx, r8
   jz			enum_case_12
   push		rcx
   mov		r8, rdx
   mov		rdx, fmt_int
   push		r8
   push		rdx
   push		rcx
   call		sprintf
   pop		rax
   pop		rbx
   pop		rbx
   pop		rbx
   ret
enum_case_0:
   mov		rax, __TypeKind.Void
   ret
enum_case_1:
   mov		rax, __TypeKind.Bool
   ret
enum_case_2:
   mov		rax, __TypeKind.Int
   ret
enum_case_3:
   mov		rax, __TypeKind.Float
   ret
enum_case_4:
   mov		rax, __TypeKind.String
   ret
enum_case_5:
   mov		rax, __TypeKind.Array
   ret
enum_case_6:
   mov		rax, __TypeKind.Pointer
   ret
enum_case_7:
   mov		rax, __TypeKind.Struct
   ret
enum_case_8:
   mov		rax, __TypeKind.Enum
   ret
enum_case_9:
   mov		rax, __TypeKind.Function
   ret
enum_case_10:
   mov		rax, __TypeKind.Tuple
   ret
enum_case_11:
   mov		rax, __TypeKind.Variadic
   ret
enum_case_12:
   mov		rax, __TypeKind.Any
   ret
get_enum_string_TypePrimitiveKind:
   mov		r8, 0
   cmp		rdx, r8
   jz			enum_case_14
   mov		r8, 1
   cmp		rdx, r8
   jz			enum_case_15
   mov		r8, 2
   cmp		rdx, r8
   jz			enum_case_16
   mov		r8, 3
   cmp		rdx, r8
   jz			enum_case_17
   mov		r8, 4
   cmp		rdx, r8
   jz			enum_case_18
   mov		r8, 5
   cmp		rdx, r8
   jz			enum_case_19
   mov		r8, 6
   cmp		rdx, r8
   jz			enum_case_20
   mov		r8, 7
   cmp		rdx, r8
   jz			enum_case_21
   mov		r8, 8
   cmp		rdx, r8
   jz			enum_case_22
   mov		r8, 9
   cmp		rdx, r8
   jz			enum_case_23
   mov		r8, 10
   cmp		rdx, r8
   jz			enum_case_24
   mov		r8, 11
   cmp		rdx, r8
   jz			enum_case_25
   mov		r8, 12
   cmp		rdx, r8
   jz			enum_case_26
   mov		r8, 13
   cmp		rdx, r8
   jz			enum_case_27
   mov		r8, 14
   cmp		rdx, r8
   jz			enum_case_28
   mov		r8, 15
   cmp		rdx, r8
   jz			enum_case_29
   mov		r8, 16
   cmp		rdx, r8
   jz			enum_case_30
   mov		r8, 17
   cmp		rdx, r8
   jz			enum_case_31
   push		rcx
   mov		r8, rdx
   mov		rdx, fmt_int
   push		r8
   push		rdx
   push		rcx
   call		sprintf
   pop		rax
   pop		rbx
   pop		rbx
   pop		rbx
   ret
enum_case_14:
   mov		rax, __TypePrimitiveKind.Uint
   ret
enum_case_15:
   mov		rax, __TypePrimitiveKind.U8
   ret
enum_case_16:
   mov		rax, __TypePrimitiveKind.U16
   ret
enum_case_17:
   mov		rax, __TypePrimitiveKind.U32
   ret
enum_case_18:
   mov		rax, __TypePrimitiveKind.U64
   ret
enum_case_19:
   mov		rax, __TypePrimitiveKind.Int
   ret
enum_case_20:
   mov		rax, __TypePrimitiveKind.S8
   ret
enum_case_21:
   mov		rax, __TypePrimitiveKind.S16
   ret
enum_case_22:
   mov		rax, __TypePrimitiveKind.S32
   ret
enum_case_23:
   mov		rax, __TypePrimitiveKind.S64
   ret
enum_case_24:
   mov		rax, __TypePrimitiveKind.Float
   ret
enum_case_25:
   mov		rax, __TypePrimitiveKind.F32
   ret
enum_case_26:
   mov		rax, __TypePrimitiveKind.F64
   ret
enum_case_27:
   mov		rax, __TypePrimitiveKind.String
   ret
enum_case_28:
   mov		rax, __TypePrimitiveKind.Bool
   ret
enum_case_29:
   mov		rax, __TypePrimitiveKind.Void
   ret
enum_case_30:
   mov		rax, __TypePrimitiveKind.UntypedInt
   ret
enum_case_31:
   mov		rax, __TypePrimitiveKind.UntypedFloat
   ret
get_enum_string_TypeArrayKind:
   mov		r8, 0
   cmp		rdx, r8
   jz			enum_case_33
   mov		r8, 1
   cmp		rdx, r8
   jz			enum_case_34
   mov		r8, 2
   cmp		rdx, r8
   jz			enum_case_35
   push		rcx
   mov		r8, rdx
   mov		rdx, fmt_int
   push		r8
   push		rdx
   push		rcx
   call		sprintf
   pop		rax
   pop		rbx
   pop		rbx
   pop		rbx
   ret
enum_case_33:
   mov		rax, __TypeArrayKind.Fixed
   ret
enum_case_34:
   mov		rax, __TypeArrayKind.Slice
   ret
enum_case_35:
   mov		rax, __TypeArrayKind.Dynamic
   ret

; bytes locals   : 24
; bytes temp     : 0
; bytes total    : 64
; (ret_0: *bool, type: *Type)
Type.isPrimitive:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64
   ; Param ret_0
   mov		-8[rbp], rcx
   ; Param type
   mov		-16[rbp], rdx
   lea		rax, -16[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   push		rax
   push		1
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L38
   ; block of if
   ; Return
   push		1
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L37
   jmp L38
; done
L38:
   lea		rax, -16[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   push		rax
   push		2
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L39
   ; block of if
   ; Return
   push		1
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L37
   jmp L39
; done
L39:
   lea		rax, -16[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   push		rax
   push		3
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L40
   ; block of if
   ; Return
   push		1
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L37
   jmp L40
; done
L40:
   lea		rax, -16[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   push		rax
   push		4
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L41
   ; block of if
   ; Return
   push		1
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L37
   jmp L41
; done
L41:
   ; Return
   push		0
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L37
L37:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret

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
   ; Ln 9: $result : string = -48[rbp]
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
   jz			L44
   ; block of if
   ; Ln 12: Assignment
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
   ; Ln 13: Assignment
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
   jmp L43
; else
L44:
   ; Ln 15: Assignment
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
   ; Ln 16: Assignment
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
   jmp L43
; done
L43:
   ; Return
   lea		rax, -48[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   jmp		L42
L42:
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
   ; Ln 23: $result : string = -40[rbp]
   mov		QWORD -40[rbp], 0
   mov		QWORD -32[rbp], 0
   ; Ln 25: Assignment
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
   ; Ln 28: Assignment
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
   jmp		L45
L45:
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
   ; Ln 34: $result : string = -40[rbp]
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
   ; Ln 36: For-loop
   lea		rax, -24[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-56[rbp], rbx     ; data
   mov		-64[rbp], rcx     ; count
   mov		QWORD -72[rbp], 0 ; index
L47:
   mov		rbx, -64[rbp]
   mov		rax, -72[rbp]
   cmp		rax, rbx
   jge		L49
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
   jz			L50
   ; block of if
   ; Ln 38: Assignment
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
   jmp L50
; done
L50:
L48:
   inc		QWORD -72[rbp]
   jmp		L47
L49:
   ; Return
   lea		rax, -40[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   jmp		L46
L46:
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
   ; Ln 46: $result : string = -40[rbp]
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
   ; Ln 48: For-loop
   lea		rax, -24[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-56[rbp], rbx     ; data
   mov		-64[rbp], rcx     ; count
   mov		QWORD -72[rbp], 0 ; index
L52:
   mov		rbx, -64[rbp]
   mov		rax, -72[rbp]
   cmp		rax, rbx
   jge		L54
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
   jz			L55
   ; block of if
   ; Ln 50: Assignment
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
   jmp L55
; done
L55:
L53:
   inc		QWORD -72[rbp]
   jmp		L52
L54:
   ; Return
   lea		rax, -40[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   jmp		L51
L51:
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
   ; Ln 58: For-loop
   lea		rax, -40[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-56[rbp], rbx     ; data
   mov		-64[rbp], rcx     ; count
   mov		QWORD -72[rbp], 0 ; index
L57:
   mov		rbx, -64[rbp]
   mov		rax, -72[rbp]
   cmp		rax, rbx
   jge		L59
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
   jz			L60
   ; block of if
   ; Return
   push		0
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L56
   jmp L60
; done
L60:
L58:
   inc		QWORD -72[rbp]
   jmp		L57
L59:
   ; Return
   push		1
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L56
L56:
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
   jz			L62
   ; block of if
   ; Return
   push		0
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L61
   jmp L62
; done
L62:
   ; Ln 72: $trailing : string = -56[rbp]
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
   ; Ln 74: For-loop
   lea		rax, -56[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-72[rbp], rbx     ; data
   mov		-80[rbp], rcx     ; count
   mov		QWORD -88[rbp], 0 ; index
L63:
   mov		rbx, -80[rbp]
   mov		rax, -88[rbp]
   cmp		rax, rbx
   jge		L65
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
   jz			L66
   ; block of if
   ; Return
   push		0
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L61
   jmp L66
; done
L66:
L64:
   inc		QWORD -88[rbp]
   jmp		L63
L65:
   ; Return
   push		1
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L61
L61:
   mov		rax, 0
   add		rsp, 240
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
   jz			L68
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
   jmp		L67
   jmp L68
; done
L68:
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
   jz			L69
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
   jmp		L67
   jmp L69
; done
L69:
   ; Ln 93: $src : string = -64[rbp]
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
   ; Ln 95: For-loop
   lea		rax, -64[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-80[rbp], rbx     ; data
   mov		-88[rbp], rcx     ; count
   mov		QWORD -96[rbp], 0 ; index
L70:
   mov		rbx, -88[rbp]
   mov		rax, -96[rbp]
   cmp		rax, rbx
   jge		L72
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
   jz			L73
   ; block of if
   ; Ln 99: $match : bool = -97[rbp]
   push		1
   pop		rax
   mov		BYTE -97[rbp], al
   ; Ln 100: For-loop
   lea		rax, -40[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-112[rbp], rbx     ; data
   mov		-120[rbp], rcx     ; count
   mov		QWORD -128[rbp], 0 ; index
L74:
   mov		rbx, -120[rbp]
   mov		rax, -128[rbp]
   cmp		rax, rbx
   jge		L76
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
   jz			L77
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
   jmp		L67
   jmp L77
; done
L77:
   ; Ln 107: $c1f : char = -129[rbp]
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
   jz			L78
   ; block of if
   ; Ln 111: Assignment
   push		0
   lea		rax, -97[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		BYTE [rbx], al
   jmp		L76
   jmp L78
; done
L78:
L75:
   inc		QWORD -128[rbp]
   jmp		L74
L76:
   movzx		eax, BYTE -97[rbp]
   push		rax
   pop		rax
   cmp		al, 0
   jz			L79
   ; block of if
   ; Return
   mov		eax, DWORD -96[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -44[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   mov		DWORD [rbx], eax
   jmp		L67
   jmp L79
; done
L79:
   jmp L73
; done
L73:
L71:
   inc		QWORD -96[rbp]
   jmp		L70
L72:
   ; Return
   mov		rax, 1
   push		rax
   pop		rax
   neg		rax
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   mov		DWORD [rbx], eax
   jmp		L67
L67:
   mov		rax, 0
   add		rsp, 288
   pop		rbp
   ret

; bytes locals   : 152
; bytes temp     : 72
; bytes total    : 256
; (ret_0: *[..]string, s: string, sep: string)
String.split:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 256
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
   ; Ln 126: $result : [..]string = -72[rbp]
   lea		rbx, -72[rbp]
   push		rbx
   mov		rdx, 16
   mov		rcx, 2
   call		calloc
   pop		rbx
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 0
   mov		QWORD 16[rbx], 2
   mov		QWORD 24[rbx], 16
   ; Ln 128: $cursor : int = -76[rbp]
   mov		rax, 0
   push		rax
   pop		rax
   mov		DWORD -76[rbp], eax
L81:
   push		1
   pop		rax
   cmp		al, 0
   jz			L82
   ; While body
   ; Ln 130: $i : int = -80[rbp]
   lea		rax, -156[rbp]
   push		rax
   lea		rax, -24[rbp]
   push		rax
   lea		rax, -40[rbp]
   push		rax
   mov		eax, DWORD -76[rbp]
   movsx		rax, eax
   push		rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, string, int) 
   call		String.indexOf
   mov		eax, DWORD -156[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -80[rbp], eax
   mov		eax, DWORD -80[rbp]
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
   jz			L84
   ; block of if
   ; Ln 132: $tail : string = -96[rbp]
   lea		rax, -168[rbp]
   push		rax
   lea		rax, -24[rbp]
   push		rax
   mov		eax, DWORD -76[rbp]
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
   lea		rax, -168[rbp]
   push		rax
   pop		rax
   lea		rbx, -96[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -72[rbp]
   push		rax
   lea		rax, -96[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; ([..]string, string) 
   call		runtime_builtin_append
   jmp		L82
   jmp L83
; else
L84:
   ; Ln 136: $segment : string = -112[rbp]
   lea		rax, -168[rbp]
   push		rax
   lea		rax, -24[rbp]
   push		rax
   mov		eax, DWORD -76[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -80[rbp]
   movsx		rax, eax
   push		rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, int, int) 
   call		String.slice
   lea		rax, -168[rbp]
   push		rax
   pop		rax
   lea		rbx, -112[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -72[rbp]
   push		rax
   lea		rax, -112[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; ([..]string, string) 
   call		runtime_builtin_append
   ; Ln 138: Assignment
   mov		eax, DWORD -80[rbp]
   movsx		rax, eax
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
   add		rax, rbx
   push		rax
   lea		rax, -76[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
   jmp L83
; done
L83:
   jmp			L81
L82:
   ; Return
   lea		rax, -72[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 32
   call		memcpy
   jmp		L80
L80:
   mov		rax, 0
   add		rsp, 256
   pop		rbp
   ret

; bytes locals   : 96
; bytes temp     : 48
; bytes total    : 176
; (ret_0: *string, s: string)
String.reverse:
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
   ; Ln 146: $result : string = -40[rbp]
   lea		rax, -112[rbp]
   push		rax
   lea		rax, -24[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (string) 
   call		String.copy
   lea		rax, -112[rbp]
   push		rax
   pop		rax
   lea		rbx, -40[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 148: For-loop
   lea		rax, -24[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-56[rbp], rbx     ; data
   mov		-64[rbp], rcx     ; count
   mov		QWORD -72[rbp], 0 ; index
L86:
   mov		rbx, -64[rbp]
   mov		rax, -72[rbp]
   cmp		rax, rbx
   jge		L88
   mov		rbx, QWORD -56[rbp]
   mov		rax, QWORD -72[rbp]
   imul		rax, 1
   lea		rbx, [rbx + rax]
   mov		al, BYTE [rbx]
   mov		-41[rbp], al 
   ; Ln 149: $j : s64 = -80[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   mov		eax, DWORD -72[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   pop		rax
   mov		QWORD -80[rbp], rax
   ; Ln 150: Assignment
   movzx		eax, BYTE -41[rbp]
   push		rax
   lea		rax, -40[rbp]
   push		rax
   mov		rax, QWORD -80[rbp]
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
L87:
   inc		QWORD -72[rbp]
   jmp		L86
L88:
   ; Return
   lea		rax, -40[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   jmp		L85
L85:
   mov		rax, 0
   add		rsp, 176
   pop		rbp
   ret

; bytes locals   : 352
; bytes temp     : 104
; bytes total    : 496
; ()
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 496
   ; Ln 2: $skip : bool = -1[rbp]
   push		0
   pop		rax
   mov		BYTE -1[rbp], al
   movzx		eax, BYTE -1[rbp]
   push		rax
   pop		rax
   test		rax, rax
   sete		al
   movzx		rax, al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L90
   ; block of if
   ; Ln 6: $text : string = -24[rbp]
   mov		rax, CS0
   mov		-368[rbp], rax
   mov		QWORD -360[rbp], 43
   lea		rax, -368[rbp]
   push		rax
   pop		rax
   lea		rbx, -24[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 8: $quickBrown : string = -40[rbp]
   lea		rax, -368[rbp]
   push		rax
   lea		rax, -24[rbp]
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
   lea		rax, -368[rbp]
   push		rax
   pop		rax
   lea		rbx, -40[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 10 Print
   lea		rax, -40[rbp]
   push		rax
   pop		rbx
   mov		-360[rbp], rbx
   mov		rdx, 8[rbx]
   add		rdx, 1
   mov		rcx, 1
   sub		rsp, 32
   call		calloc
   add		rsp, 32
   mov		rbx, -360[rbp]
   mov		rdx, 0[rbx]
   mov		r8d, 8[rbx]
   mov		-360[rbp], rax
   mov		rcx, rax
   sub		rsp, 32
   call		memcpy
   add		rsp, 32
   mov		rax, -360[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS1
   call		printf
   ; Ln 11 Print
   lea		rax, -40[rbp]
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
   ; Ln 13: $fox : string = -56[rbp]
   lea		rax, -368[rbp]
   push		rax
   lea		rax, -24[rbp]
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
   lea		rax, -368[rbp]
   push		rax
   pop		rax
   lea		rbx, -56[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 14 Print
   lea		rax, -56[rbp]
   push		rax
   pop		rbx
   mov		-360[rbp], rbx
   mov		rdx, 8[rbx]
   add		rdx, 1
   mov		rcx, 1
   sub		rsp, 32
   call		calloc
   add		rsp, 32
   mov		rbx, -360[rbp]
   mov		rdx, 0[rbx]
   mov		r8d, 8[rbx]
   mov		-360[rbp], rax
   mov		rcx, rax
   sub		rsp, 32
   call		memcpy
   add		rsp, 32
   mov		rax, -360[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS3
   call		printf
   ; Ln 15 Print
   lea		rax, -56[rbp]
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
   jmp L90
; done
L90:
   movzx		eax, BYTE -1[rbp]
   push		rax
   pop		rax
   test		rax, rax
   sete		al
   movzx		rax, al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L91
   ; block of if
   ; Ln 19: $text : string = -72[rbp]
   mov		rax, CS5
   mov		-368[rbp], rax
   mov		QWORD -360[rbp], 43
   lea		rax, -368[rbp]
   push		rax
   pop		rax
   lea		rbx, -72[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 21: $copy : string = -88[rbp]
   lea		rax, -368[rbp]
   push		rax
   lea		rax, -72[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (string) 
   call		String.copy
   lea		rax, -368[rbp]
   push		rax
   pop		rax
   lea		rbx, -88[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 23: Assignment
   mov		rax, 115
   push		rax
   lea		rax, -72[rbp]
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
   ; Ln 24: Assignment
   mov		rax, 108
   push		rax
   lea		rax, -72[rbp]
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
   ; Ln 25: Assignment
   mov		rax, 111
   push		rax
   lea		rax, -72[rbp]
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
   ; Ln 26: Assignment
   mov		rax, 119
   push		rax
   lea		rax, -72[rbp]
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
   ; Ln 27: Assignment
   mov		rax, 32
   push		rax
   lea		rax, -72[rbp]
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
   ; Ln 28 Print
   lea		rax, -72[rbp]
   push		rax
   pop		rbx
   mov		-360[rbp], rbx
   mov		rdx, 8[rbx]
   add		rdx, 1
   mov		rcx, 1
   sub		rsp, 32
   call		calloc
   add		rsp, 32
   mov		rbx, -360[rbp]
   mov		rdx, 0[rbx]
   mov		r8d, 8[rbx]
   mov		-360[rbp], rax
   mov		rcx, rax
   sub		rsp, 32
   call		memcpy
   add		rsp, 32
   mov		rax, -360[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS6
   call		printf
   ; Ln 30 Print
   lea		rax, -88[rbp]
   push		rax
   pop		rbx
   mov		-360[rbp], rbx
   mov		rdx, 8[rbx]
   add		rdx, 1
   mov		rcx, 1
   sub		rsp, 32
   call		calloc
   add		rsp, 32
   mov		rbx, -360[rbp]
   mov		rdx, 0[rbx]
   mov		r8d, 8[rbx]
   mov		-360[rbp], rax
   mov		rcx, rax
   sub		rsp, 32
   call		memcpy
   add		rsp, 32
   mov		rax, -360[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS7
   call		printf
   jmp L91
; done
L91:
   movzx		eax, BYTE -1[rbp]
   push		rax
   pop		rax
   test		rax, rax
   sete		al
   movzx		rax, al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L92
   ; block of if
   ; Ln 34: $text : string = -104[rbp]
   mov		rax, CS8
   mov		-368[rbp], rax
   mov		QWORD -360[rbp], 43
   lea		rax, -368[rbp]
   push		rax
   pop		rax
   lea		rbx, -104[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 36: $uppercased : string = -120[rbp]
   lea		rax, -368[rbp]
   push		rax
   lea		rax, -104[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (string) 
   call		String.toUpper
   lea		rax, -368[rbp]
   push		rax
   pop		rax
   lea		rbx, -120[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 37 Print
   lea		rax, -120[rbp]
   push		rax
   pop		rbx
   mov		-360[rbp], rbx
   mov		rdx, 8[rbx]
   add		rdx, 1
   mov		rcx, 1
   sub		rsp, 32
   call		calloc
   add		rsp, 32
   mov		rbx, -360[rbp]
   mov		rdx, 0[rbx]
   mov		r8d, 8[rbx]
   mov		-360[rbp], rax
   mov		rcx, rax
   sub		rsp, 32
   call		memcpy
   add		rsp, 32
   mov		rax, -360[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS9
   call		printf
   jmp L92
; done
L92:
   movzx		eax, BYTE -1[rbp]
   push		rax
   pop		rax
   test		rax, rax
   sete		al
   movzx		rax, al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L93
   ; block of if
   ; Ln 42: $text : string = -136[rbp]
   mov		rax, CS10
   mov		-368[rbp], rax
   mov		QWORD -360[rbp], 43
   lea		rax, -368[rbp]
   push		rax
   pop		rax
   lea		rbx, -136[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 44: $lowercase : string = -152[rbp]
   lea		rax, -368[rbp]
   push		rax
   lea		rax, -136[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (string) 
   call		String.toLower
   lea		rax, -368[rbp]
   push		rax
   pop		rax
   lea		rbx, -152[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 45 Print
   lea		rax, -152[rbp]
   push		rax
   pop		rbx
   mov		-360[rbp], rbx
   mov		rdx, 8[rbx]
   add		rdx, 1
   mov		rcx, 1
   sub		rsp, 32
   call		calloc
   add		rsp, 32
   mov		rbx, -360[rbp]
   mov		rdx, 0[rbx]
   mov		r8d, 8[rbx]
   mov		-360[rbp], rax
   mov		rcx, rax
   sub		rsp, 32
   call		memcpy
   add		rsp, 32
   mov		rax, -360[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS11
   call		printf
   jmp L93
; done
L93:
   movzx		eax, BYTE -1[rbp]
   push		rax
   pop		rax
   test		rax, rax
   sete		al
   movzx		rax, al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L94
   ; block of if
   ; Ln 50: $text : string = -168[rbp]
   mov		rax, CS12
   mov		-368[rbp], rax
   mov		QWORD -360[rbp], 43
   lea		rax, -368[rbp]
   push		rax
   pop		rax
   lea		rbx, -168[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -353[rbp]
   push		rax
   lea		rax, -168[rbp]
   push		rax
   mov		rax, CS13
   mov		-369[rbp], rax
   mov		QWORD -361[rbp], 3
   lea		rax, -369[rbp]
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, string) 
   call		String.startsWith
   movzx		eax, BYTE -353[rbp]
   push		rax
   pop		rcx
   mov		rdx, 51
   call		assert
   jmp L94
; done
L94:
   movzx		eax, BYTE -1[rbp]
   push		rax
   pop		rax
   test		rax, rax
   sete		al
   movzx		rax, al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L95
   ; block of if
   ; Ln 56: $text : string = -184[rbp]
   mov		rax, CS14
   mov		-368[rbp], rax
   mov		QWORD -360[rbp], 43
   lea		rax, -368[rbp]
   push		rax
   pop		rax
   lea		rbx, -184[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -353[rbp]
   push		rax
   lea		rax, -184[rbp]
   push		rax
   mov		rax, CS15
   mov		-369[rbp], rax
   mov		QWORD -361[rbp], 3
   lea		rax, -369[rbp]
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, string) 
   call		String.endsWith
   movzx		eax, BYTE -353[rbp]
   push		rax
   pop		rcx
   mov		rdx, 57
   call		assert
   jmp L95
; done
L95:
   movzx		eax, BYTE -1[rbp]
   push		rax
   pop		rax
   test		rax, rax
   sete		al
   movzx		rax, al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L96
   ; block of if
   ; Ln 62: $text : string = -200[rbp]
   mov		rax, CS16
   mov		-368[rbp], rax
   mov		QWORD -360[rbp], 43
   lea		rax, -368[rbp]
   push		rax
   pop		rax
   lea		rbx, -200[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 64: $index1 : int = -204[rbp]
   lea		rax, -356[rbp]
   push		rax
   lea		rax, -200[rbp]
   push		rax
   mov		rax, CS17
   mov		-372[rbp], rax
   mov		QWORD -364[rbp], 5
   lea		rax, -372[rbp]
   push		rax
   mov		rax, 0
   push		rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, string, int) 
   call		String.indexOf
   mov		eax, DWORD -356[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -204[rbp], eax
   mov		eax, DWORD -204[rbp]
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
   mov		rdx, 65
   call		assert
   ; Ln 67: $index2 : int = -208[rbp]
   lea		rax, -356[rbp]
   push		rax
   lea		rax, -200[rbp]
   push		rax
   mov		rax, CS18
   mov		-372[rbp], rax
   mov		QWORD -364[rbp], 3
   lea		rax, -372[rbp]
   push		rax
   mov		rax, 0
   push		rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, string, int) 
   call		String.indexOf
   mov		eax, DWORD -356[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -208[rbp], eax
   mov		eax, DWORD -208[rbp]
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
   mov		rdx, 68
   call		assert
   ; Ln 71: $index3 : int = -212[rbp]
   lea		rax, -356[rbp]
   push		rax
   lea		rax, -200[rbp]
   push		rax
   mov		rax, CS19
   mov		-372[rbp], rax
   mov		QWORD -364[rbp], 5
   lea		rax, -372[rbp]
   push		rax
   mov		rax, 10
   push		rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, string, int) 
   call		String.indexOf
   mov		eax, DWORD -356[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -212[rbp], eax
   mov		eax, DWORD -212[rbp]
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
   mov		rdx, 72
   call		assert
   ; Ln 74: $index4 : int = -216[rbp]
   lea		rax, -356[rbp]
   push		rax
   lea		rax, -200[rbp]
   push		rax
   mov		rax, CS20
   mov		-372[rbp], rax
   mov		QWORD -364[rbp], 3
   lea		rax, -372[rbp]
   push		rax
   mov		rax, 4
   push		rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, string, int) 
   call		String.indexOf
   mov		eax, DWORD -356[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -216[rbp], eax
   mov		eax, DWORD -216[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 31
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 75
   call		assert
   jmp L96
; done
L96:
   push		1
   pop		rax
   cmp		al, 0
   jz			L97
   ; block of if
   ; Ln 80: $text : string = -232[rbp]
   mov		rax, CS21
   mov		-368[rbp], rax
   mov		QWORD -360[rbp], 43
   lea		rax, -368[rbp]
   push		rax
   pop		rax
   lea		rbx, -232[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 82: $splitted : [..]string = -264[rbp]
   lea		rax, -384[rbp]
   push		rax
   lea		rax, -232[rbp]
   push		rax
   mov		rax, CS22
   mov		-400[rbp], rax
   mov		QWORD -392[rbp], 1
   lea		rax, -400[rbp]
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, string) 
   call		String.split
   lea		rax, -384[rbp]
   push		rax
   pop		rax
   lea		rbx, -264[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 32
   call		memcpy
   lea		rax, -264[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   mov		rax, 9
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 84
   call		assert
   ; Ln 86: For-loop
   lea		rax, -264[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-288[rbp], rbx     ; data
   mov		-296[rbp], rcx     ; count
   mov		QWORD -304[rbp], 0 ; index
L98:
   mov		rbx, -296[rbp]
   mov		rax, -304[rbp]
   cmp		rax, rbx
   jge		L100
   mov		rbx, QWORD -288[rbp]
   mov		rax, QWORD -304[rbp]
   imul		rax, 16
   lea		rbx, [rbx + rax]
   mov		r8, 16
   mov		rdx, rbx
   lea		rcx, -280[rbp]
   call		memcpy
   ; Ln 87 Print
   lea		rax, -280[rbp]
   push		rax
   pop		rbx
   mov		-360[rbp], rbx
   mov		rdx, 8[rbx]
   add		rdx, 1
   mov		rcx, 1
   sub		rsp, 32
   call		calloc
   add		rsp, 32
   mov		rbx, -360[rbp]
   mov		rdx, 0[rbx]
   mov		r8d, 8[rbx]
   mov		-360[rbp], rax
   mov		rcx, rax
   sub		rsp, 32
   call		memcpy
   add		rsp, 32
   mov		rax, -360[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS23
   call		printf
L99:
   inc		QWORD -304[rbp]
   jmp		L98
L100:
   jmp L97
; done
L97:
   push		1
   pop		rax
   cmp		al, 0
   jz			L101
   ; block of if
   ; Ln 93: $text : string = -320[rbp]
   mov		rax, CS24
   mov		-368[rbp], rax
   mov		QWORD -360[rbp], 43
   lea		rax, -368[rbp]
   push		rax
   pop		rax
   lea		rbx, -320[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 95: $reversed : string = -336[rbp]
   lea		rax, -368[rbp]
   push		rax
   lea		rax, -320[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (string) 
   call		String.reverse
   lea		rax, -368[rbp]
   push		rax
   pop		rax
   lea		rbx, -336[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -336[rbp]
   push		rax
   mov		rax, CS25
   mov		-368[rbp], rax
   mov		QWORD -360[rbp], 43
   lea		rax, -368[rbp]
   push		rax
   pop		rdx
   pop		rcx
   sub		rsp, 32
   call		runtime_compare_strings
   add		rsp, 32
   cmp		eax, 0
   sete		al
   movzx		eax, al
   push		rax
   pop		rcx
   mov		rdx, 97
   call		assert
   jmp L101
; done
L101:
L89:
   mov		rax, 0
   add		rsp, 496
   pop		rbp
   ret
