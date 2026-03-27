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
   CS0 DB `OutOfMemoryException: Failed to reallocate a new buffer for the string builder`, 0 
   CS1 DB `%f`, 0 
   CS2 DB `%lf`, 0 
   CS3 DB `UNHANDLED`, 0 
   CS4 DB `true`, 0 
   CS5 DB `false`, 0 
   CS6 DB `.`, 0 
   CS7 DB `0x%p`, 0 
   CS8 DB `[]`, 0 
   CS9 DB `[`, 0 
   CS10 DB `0x%p`, 0 
   CS11 DB `, `, 0 
   CS12 DB `]`, 0 
   CS13 DB `{ `, 0 
   CS14 DB ` = `, 0 
   CS15 DB `, `, 0 
   CS16 DB ` }`, 0 
   CS17 DB `<`, 0 
   CS18 DB `>`, 0 
   CS19 DB `%s\n`, 0 
   CS20 DB `%s`, 0 
   CS25 DB `%`, 0 
   CS26 DB `r`, 0 
   CS27 DB `g`, 0 
   CS28 DB `b`, 0 
   CS29 DB `a`, 0 
   CS30 DB `Color`, 0 
   CS31 DB `%`, 0 
   CS32 DB `r`, 0 
   CS33 DB `g`, 0 
   CS34 DB `b`, 0 
   CS35 DB `a`, 0 
   CS36 DB `Color`, 0 
   CS37 DB `%`, 0 
   CS38 DB `r`, 0 
   CS39 DB `g`, 0 
   CS40 DB `b`, 0 
   CS41 DB `a`, 0 
   CS42 DB `Color`, 0 
   CS43 DB `%`, 0 
   CS44 DB `r`, 0 
   CS45 DB `g`, 0 
   CS46 DB `b`, 0 
   CS47 DB `a`, 0 
   CS48 DB `Color`, 0 
segment .rdata
segment .rdata
   TypeKind.name.data DB "TypeKind", 0
   align 8
   TypeKind.name:
   dq TypeKind.name.data
   dq 8
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
   align 8
   TypeKind.names.array:
   dq __TypeKind.Void
   dq 4
   dq __TypeKind.Bool
   dq 4
   dq __TypeKind.Int
   dq 3
   dq __TypeKind.Float
   dq 5
   dq __TypeKind.String
   dq 6
   dq __TypeKind.Array
   dq 5
   dq __TypeKind.Pointer
   dq 7
   dq __TypeKind.Struct
   dq 6
   dq __TypeKind.Enum
   dq 4
   dq __TypeKind.Function
   dq 8
   dq __TypeKind.Tuple
   dq 5
   dq __TypeKind.Variadic
   dq 8
   dq __TypeKind.Any
   dq 3
   align 8
   TypeKind.names:
   dq TypeKind.names.array:
   dq 13
   align 4
   TypeKind.values.array:
   dd 0
   dd 1
   dd 2
   dd 3
   dd 4
   dd 5
   dd 6
   dd 7
   dd 8
   dd 9
   dd 10
   dd 11
   dd 12
   align 8
   TypeKind.values:
   dq TypeKind.values.array:
   dq 13
   TypePrimitiveKind.name.data DB "TypePrimitiveKind", 0
   align 8
   TypePrimitiveKind.name:
   dq TypePrimitiveKind.name.data
   dq 17
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
   align 8
   TypePrimitiveKind.names.array:
   dq __TypePrimitiveKind.Uint
   dq 4
   dq __TypePrimitiveKind.U8
   dq 2
   dq __TypePrimitiveKind.U16
   dq 3
   dq __TypePrimitiveKind.U32
   dq 3
   dq __TypePrimitiveKind.U64
   dq 3
   dq __TypePrimitiveKind.Int
   dq 3
   dq __TypePrimitiveKind.S8
   dq 2
   dq __TypePrimitiveKind.S16
   dq 3
   dq __TypePrimitiveKind.S32
   dq 3
   dq __TypePrimitiveKind.S64
   dq 3
   dq __TypePrimitiveKind.Float
   dq 5
   dq __TypePrimitiveKind.F32
   dq 3
   dq __TypePrimitiveKind.F64
   dq 3
   dq __TypePrimitiveKind.String
   dq 6
   dq __TypePrimitiveKind.Bool
   dq 4
   dq __TypePrimitiveKind.Void
   dq 4
   dq __TypePrimitiveKind.UntypedInt
   dq 10
   dq __TypePrimitiveKind.UntypedFloat
   dq 12
   align 8
   TypePrimitiveKind.names:
   dq TypePrimitiveKind.names.array:
   dq 18
   align 4
   TypePrimitiveKind.values.array:
   dd 0
   dd 1
   dd 2
   dd 3
   dd 4
   dd 5
   dd 6
   dd 7
   dd 8
   dd 9
   dd 10
   dd 11
   dd 12
   dd 13
   dd 14
   dd 15
   dd 16
   dd 17
   align 8
   TypePrimitiveKind.values:
   dq TypePrimitiveKind.values.array:
   dq 18
   TypeArrayKind.name.data DB "TypeArrayKind", 0
   align 8
   TypeArrayKind.name:
   dq TypeArrayKind.name.data
   dq 13
   __TypeArrayKind.Fixed DB "Fixed", 0
   __TypeArrayKind.Slice DB "Slice", 0
   __TypeArrayKind.Dynamic DB "Dynamic", 0
   align 8
   TypeArrayKind.names.array:
   dq __TypeArrayKind.Fixed
   dq 5
   dq __TypeArrayKind.Slice
   dq 5
   dq __TypeArrayKind.Dynamic
   dq 7
   align 8
   TypeArrayKind.names:
   dq TypeArrayKind.names.array:
   dq 3
   align 4
   TypeArrayKind.values.array:
   dd 0
   dd 1
   dd 2
   align 8
   TypeArrayKind.values:
   dq TypeArrayKind.values.array:
   dq 3
   Red.22:
   db 255
   times 2 db 0
   db 255
   Green.23:
   times 1 db 0
   db 255
   times 1 db 0
   db 255
   Blue.24:
   times 2 db 0
   db 255
   db 255
   Rainbow.25:
   db 255
   times 2 db 0
   db 255
   times 1 db 0
   db 255
   times 1 db 0
   db 255
   times 2 db 0
   db 255
   db 255
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
   extern strlen
   extern sprintf
   extern printf




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
   mov		r9, 0
   cmp		r8, r9
   jz			enum_case_0
   mov		r9, 1
   cmp		r8, r9
   jz			enum_case_1
   mov		r9, 2
   cmp		r8, r9
   jz			enum_case_2
   mov		r9, 3
   cmp		r8, r9
   jz			enum_case_3
   mov		r9, 4
   cmp		r8, r9
   jz			enum_case_4
   mov		r9, 5
   cmp		r8, r9
   jz			enum_case_5
   mov		r9, 6
   cmp		r8, r9
   jz			enum_case_6
   mov		r9, 7
   cmp		r8, r9
   jz			enum_case_7
   mov		r9, 8
   cmp		r8, r9
   jz			enum_case_8
   mov		r9, 9
   cmp		r8, r9
   jz			enum_case_9
   mov		r9, 10
   cmp		r8, r9
   jz			enum_case_10
   mov		r9, 11
   cmp		r8, r9
   jz			enum_case_11
   mov		r9, 12
   cmp		r8, r9
   jz			enum_case_12
   push		rcx
   push		rdx
   mov		rcx, rdx
   mov		rdx, fmt_int
   sub		rsp, 32
   call		sprintf
   add		rsp, 32
   pop		rdx
   pop		rcx
   mov		0[rcx], rdx
   mov		8[rcx], rax
   mov		rax, rcx
   ret
enum_case_0:
   mov		rax, __TypeKind.Void
   mov		QWORD rbx, 4
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_1:
   mov		rax, __TypeKind.Bool
   mov		QWORD rbx, 4
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_2:
   mov		rax, __TypeKind.Int
   mov		QWORD rbx, 3
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_3:
   mov		rax, __TypeKind.Float
   mov		QWORD rbx, 5
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_4:
   mov		rax, __TypeKind.String
   mov		QWORD rbx, 6
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_5:
   mov		rax, __TypeKind.Array
   mov		QWORD rbx, 5
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_6:
   mov		rax, __TypeKind.Pointer
   mov		QWORD rbx, 7
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_7:
   mov		rax, __TypeKind.Struct
   mov		QWORD rbx, 6
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_8:
   mov		rax, __TypeKind.Enum
   mov		QWORD rbx, 4
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_9:
   mov		rax, __TypeKind.Function
   mov		QWORD rbx, 8
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_10:
   mov		rax, __TypeKind.Tuple
   mov		QWORD rbx, 5
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_11:
   mov		rax, __TypeKind.Variadic
   mov		QWORD rbx, 8
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_12:
   mov		rax, __TypeKind.Any
   mov		QWORD rbx, 3
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
get_enum_string_TypePrimitiveKind:
   mov		r9, 0
   cmp		r8, r9
   jz			enum_case_14
   mov		r9, 1
   cmp		r8, r9
   jz			enum_case_15
   mov		r9, 2
   cmp		r8, r9
   jz			enum_case_16
   mov		r9, 3
   cmp		r8, r9
   jz			enum_case_17
   mov		r9, 4
   cmp		r8, r9
   jz			enum_case_18
   mov		r9, 5
   cmp		r8, r9
   jz			enum_case_19
   mov		r9, 6
   cmp		r8, r9
   jz			enum_case_20
   mov		r9, 7
   cmp		r8, r9
   jz			enum_case_21
   mov		r9, 8
   cmp		r8, r9
   jz			enum_case_22
   mov		r9, 9
   cmp		r8, r9
   jz			enum_case_23
   mov		r9, 10
   cmp		r8, r9
   jz			enum_case_24
   mov		r9, 11
   cmp		r8, r9
   jz			enum_case_25
   mov		r9, 12
   cmp		r8, r9
   jz			enum_case_26
   mov		r9, 13
   cmp		r8, r9
   jz			enum_case_27
   mov		r9, 14
   cmp		r8, r9
   jz			enum_case_28
   mov		r9, 15
   cmp		r8, r9
   jz			enum_case_29
   mov		r9, 16
   cmp		r8, r9
   jz			enum_case_30
   mov		r9, 17
   cmp		r8, r9
   jz			enum_case_31
   push		rcx
   push		rdx
   mov		rcx, rdx
   mov		rdx, fmt_int
   sub		rsp, 32
   call		sprintf
   add		rsp, 32
   pop		rdx
   pop		rcx
   mov		0[rcx], rdx
   mov		8[rcx], rax
   mov		rax, rcx
   ret
enum_case_14:
   mov		rax, __TypePrimitiveKind.Uint
   mov		QWORD rbx, 4
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_15:
   mov		rax, __TypePrimitiveKind.U8
   mov		QWORD rbx, 2
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_16:
   mov		rax, __TypePrimitiveKind.U16
   mov		QWORD rbx, 3
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_17:
   mov		rax, __TypePrimitiveKind.U32
   mov		QWORD rbx, 3
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_18:
   mov		rax, __TypePrimitiveKind.U64
   mov		QWORD rbx, 3
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_19:
   mov		rax, __TypePrimitiveKind.Int
   mov		QWORD rbx, 3
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_20:
   mov		rax, __TypePrimitiveKind.S8
   mov		QWORD rbx, 2
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_21:
   mov		rax, __TypePrimitiveKind.S16
   mov		QWORD rbx, 3
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_22:
   mov		rax, __TypePrimitiveKind.S32
   mov		QWORD rbx, 3
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_23:
   mov		rax, __TypePrimitiveKind.S64
   mov		QWORD rbx, 3
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_24:
   mov		rax, __TypePrimitiveKind.Float
   mov		QWORD rbx, 5
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_25:
   mov		rax, __TypePrimitiveKind.F32
   mov		QWORD rbx, 3
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_26:
   mov		rax, __TypePrimitiveKind.F64
   mov		QWORD rbx, 3
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_27:
   mov		rax, __TypePrimitiveKind.String
   mov		QWORD rbx, 6
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_28:
   mov		rax, __TypePrimitiveKind.Bool
   mov		QWORD rbx, 4
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_29:
   mov		rax, __TypePrimitiveKind.Void
   mov		QWORD rbx, 4
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_30:
   mov		rax, __TypePrimitiveKind.UntypedInt
   mov		QWORD rbx, 10
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_31:
   mov		rax, __TypePrimitiveKind.UntypedFloat
   mov		QWORD rbx, 12
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
get_enum_string_TypeArrayKind:
   mov		r9, 0
   cmp		r8, r9
   jz			enum_case_33
   mov		r9, 1
   cmp		r8, r9
   jz			enum_case_34
   mov		r9, 2
   cmp		r8, r9
   jz			enum_case_35
   push		rcx
   push		rdx
   mov		rcx, rdx
   mov		rdx, fmt_int
   sub		rsp, 32
   call		sprintf
   add		rsp, 32
   pop		rdx
   pop		rcx
   mov		0[rcx], rdx
   mov		8[rcx], rax
   mov		rax, rcx
   ret
enum_case_33:
   mov		rax, __TypeArrayKind.Fixed
   mov		QWORD rbx, 5
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_34:
   mov		rax, __TypeArrayKind.Slice
   mov		QWORD rbx, 5
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
   ret
enum_case_35:
   mov		rax, __TypeArrayKind.Dynamic
   mov		QWORD rbx, 7
   mov		QWORD 0[rcx], rax
   mov		QWORD 8[rcx], rbx
   mov		rax, rcx
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
   add		rbx, 16
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
   add		rbx, 16
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
   add		rbx, 16
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

; bytes locals   : 48
; bytes temp     : 64
; bytes total    : 144
; (ret_0: **u8, s: string)
String.toCString:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 144
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
   ; Ln 5: $cstr : *u8 = -32[rbp]
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
   add		rax, rbx
   push		rax
   pop		rcx
   ; (s64) 
   call		malloc
   push		rax
   pop		rax
   mov		QWORD -32[rbp], rax
   mov		rax, -32[rbp]
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
   ; (*u8, *char, s64) 
   call		memcpy
   push		rax
   pop		rax
   ; Ln 7: Assignment
   mov		rax, 0
   push		rax
   mov		rax, -32[rbp]
   push		rax
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   pop		rax
   pop		rbx
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   pop		rbx
   pop		rax
   mov		BYTE [rbx], al
   ; Return
   mov		rax, -32[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   mov		QWORD [rbx], rax
   jmp		L41
L41:
   mov		rax, 0
   add		rsp, 144
   pop		rbp
   ret

; bytes locals   : 40
; bytes temp     : 32
; bytes total    : 112
; (ret_0: *string, cstr: *u8)
fromCString:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 112
   ; Param ret_0
   mov		-8[rbp], rcx
   ; Param cstr
   mov		-16[rbp], rdx
   ; Ln 12: $result : string = -32[rbp]
   mov		QWORD -32[rbp], 0
   mov		QWORD -24[rbp], 0
   ; Ln 13: Assignment
   mov		rax, -16[rbp]
   push		rax
   lea		rax, -32[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   ; Ln 14: Assignment
   mov		rax, -16[rbp]
   push		rax
   pop		rcx
   ; (*u8) 
   call		strlen
   push		rax
   lea		rax, -32[rbp]
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
   lea		rax, -32[rbp]
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
   add		rsp, 112
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
   ; Ln 19: $result : string = -48[rbp]
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
   jz			L45
   ; block of if
   ; Ln 22: Assignment
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
   ; Ln 23: Assignment
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
   jmp L44
; else
L45:
   ; Ln 25: Assignment
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
   ; Ln 26: Assignment
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
   jmp L44
; done
L44:
   ; Return
   lea		rax, -48[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   jmp		L43
L43:
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
   ; Ln 33: $result : string = -40[rbp]
   mov		QWORD -40[rbp], 0
   mov		QWORD -32[rbp], 0
   ; Ln 35: Assignment
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
   ; Ln 38: Assignment
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
   jmp		L46
L46:
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
   ; Ln 44: $result : string = -40[rbp]
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
   ; Ln 46: For-loop
   lea		rax, -24[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-56[rbp], rbx     ; data
   mov		-64[rbp], rcx     ; count
   mov		QWORD -72[rbp], 0 ; index
L48:
   mov		rbx, -64[rbp]
   mov		rax, -72[rbp]
   cmp		rax, rbx
   jge		L50
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
   jz			L51
   ; block of if
   ; Ln 48: Assignment
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
   jmp L51
; done
L51:
L49:
   inc		QWORD -72[rbp]
   jmp		L48
L50:
   ; Return
   lea		rax, -40[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   jmp		L47
L47:
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
   ; Ln 56: $result : string = -40[rbp]
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
   ; Ln 58: For-loop
   lea		rax, -24[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-56[rbp], rbx     ; data
   mov		-64[rbp], rcx     ; count
   mov		QWORD -72[rbp], 0 ; index
L53:
   mov		rbx, -64[rbp]
   mov		rax, -72[rbp]
   cmp		rax, rbx
   jge		L55
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
   jz			L56
   ; block of if
   ; Ln 60: Assignment
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
   jmp L56
; done
L56:
L54:
   inc		QWORD -72[rbp]
   jmp		L53
L55:
   ; Return
   lea		rax, -40[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   jmp		L52
L52:
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
   ; Ln 68: For-loop
   lea		rax, -40[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-56[rbp], rbx     ; data
   mov		-64[rbp], rcx     ; count
   mov		QWORD -72[rbp], 0 ; index
L58:
   mov		rbx, -64[rbp]
   mov		rax, -72[rbp]
   cmp		rax, rbx
   jge		L60
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
   jz			L61
   ; block of if
   ; Return
   push		0
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L57
   jmp L61
; done
L61:
L59:
   inc		QWORD -72[rbp]
   jmp		L58
L60:
   ; Return
   push		1
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L57
L57:
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
   jz			L63
   ; block of if
   ; Return
   push		0
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L62
   jmp L63
; done
L63:
   ; Ln 82: $trailing : string = -56[rbp]
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
   ; Ln 84: For-loop
   lea		rax, -56[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-72[rbp], rbx     ; data
   mov		-80[rbp], rcx     ; count
   mov		QWORD -88[rbp], 0 ; index
L64:
   mov		rbx, -80[rbp]
   mov		rax, -88[rbp]
   cmp		rax, rbx
   jge		L66
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
   jz			L67
   ; block of if
   ; Return
   push		0
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L62
   jmp L67
; done
L67:
L65:
   inc		QWORD -88[rbp]
   jmp		L64
L66:
   ; Return
   push		1
   pop		rax
   mov		rbx, -8[rbp]
   mov		BYTE [rbx], al
   jmp		L62
L62:
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
   jmp		L68
   jmp L69
; done
L69:
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
   jz			L70
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
   jmp		L68
   jmp L70
; done
L70:
   ; Ln 103: $src : string = -64[rbp]
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
   ; Ln 105: For-loop
   lea		rax, -64[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-80[rbp], rbx     ; data
   mov		-88[rbp], rcx     ; count
   mov		QWORD -96[rbp], 0 ; index
L71:
   mov		rbx, -88[rbp]
   mov		rax, -96[rbp]
   cmp		rax, rbx
   jge		L73
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
   jz			L74
   ; block of if
   ; Ln 109: $match : bool = -97[rbp]
   push		1
   pop		rax
   mov		BYTE -97[rbp], al
   ; Ln 110: For-loop
   lea		rax, -40[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-112[rbp], rbx     ; data
   mov		-120[rbp], rcx     ; count
   mov		QWORD -128[rbp], 0 ; index
L75:
   mov		rbx, -120[rbp]
   mov		rax, -128[rbp]
   cmp		rax, rbx
   jge		L77
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
   jz			L78
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
   jmp		L68
   jmp L78
; done
L78:
   ; Ln 117: $c1f : char = -129[rbp]
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
   jz			L79
   ; block of if
   ; Ln 121: Assignment
   push		0
   lea		rax, -97[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		BYTE [rbx], al
   jmp		L77
   jmp L79
; done
L79:
L76:
   inc		QWORD -128[rbp]
   jmp		L75
L77:
   movzx		eax, BYTE -97[rbp]
   push		rax
   pop		rax
   cmp		al, 0
   jz			L80
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
   jmp		L68
   jmp L80
; done
L80:
   jmp L74
; done
L74:
L72:
   inc		QWORD -96[rbp]
   jmp		L71
L73:
   ; Return
   mov		rax, 1
   push		rax
   pop		rax
   neg		rax
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   mov		DWORD [rbx], eax
   jmp		L68
L68:
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
   ; Ln 136: $result : [..]string = -72[rbp]
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
   ; Ln 138: $cursor : int = -76[rbp]
   mov		rax, 0
   push		rax
   pop		rax
   mov		DWORD -76[rbp], eax
L82:
   push		1
   pop		rax
   cmp		al, 0
   jz			L83
   ; While body
   ; Ln 140: $i : int = -80[rbp]
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
   jz			L85
   ; block of if
   ; Ln 142: $tail : string = -96[rbp]
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
   push		0
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*[..]string, string, bool) 
   call		runtime_builtin_append
   jmp		L83
   jmp L84
; else
L85:
   ; Ln 146: $segment : string = -112[rbp]
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
   push		0
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*[..]string, string, bool) 
   call		runtime_builtin_append
   ; Ln 148: Assignment
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
   pop		rax
   mov		eax, eax
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
   jmp L84
; done
L84:
   jmp			L82
L83:
   ; Return
   lea		rax, -72[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 32
   call		memcpy
   jmp		L81
L81:
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
   ; Ln 156: $result : string = -40[rbp]
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
   ; Ln 158: For-loop
   lea		rax, -24[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-56[rbp], rbx     ; data
   mov		-64[rbp], rcx     ; count
   mov		QWORD -72[rbp], 0 ; index
L87:
   mov		rbx, -64[rbp]
   mov		rax, -72[rbp]
   cmp		rax, rbx
   jge		L89
   mov		rbx, QWORD -56[rbp]
   mov		rax, QWORD -72[rbp]
   imul		rax, 1
   lea		rbx, [rbx + rax]
   mov		al, BYTE [rbx]
   mov		-41[rbp], al 
   ; Ln 159: $j : s64 = -80[rbp]
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
   ; Ln 160: Assignment
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
L88:
   inc		QWORD -72[rbp]
   jmp		L87
L89:
   ; Return
   lea		rax, -40[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   jmp		L86
L86:
   mov		rax, 0
   add		rsp, 176
   pop		rbp
   ret

; bytes locals   : 72
; bytes temp     : 48
; bytes total    : 160
; (array: *[..]int, elem: int)
appendInt:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 160
   ; Param array
   mov		-8[rbp], rcx
   ; Param elem
   mov		-12[rbp], edx
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
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
   add		rax, rbx
   push		rax
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 16
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setg		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L91
   ; block of if
   ; Ln 8: $new_cap : s64 = -24[rbp]
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 16
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   mov		rax, 2
   push		rax
   pop		rbx
   pop		rax
   imul		rax, rbx
   push		rax
   pop		rax
   mov		QWORD -24[rbp], rax
   ; Ln 9: Assignment
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   mov		rax, QWORD -24[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*int, s64) 
   call		realloc
   push		rax
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   ; Ln 10: Assignment
   mov		rax, QWORD -24[rbp]
   push		rax
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 16
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   jmp L91
; done
L91:
   ; Ln 13: $arr : [..]int = -56[rbp]
   mov		rax, -8[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rax
   lea		rbx, -56[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 32
   call		memcpy
   ; Ln 14: Assignment
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax
   lea		rax, -56[rbp]
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
   mov		rax, QWORD [rbx]
   push		rax
   pop		rax
   pop		rbx
   mov		rbx, [rbx]
   imul		rax, 4; Add index expr
   add		rbx, rax
   push		rbx
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
   ; Ln 15: Assignment
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
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
   add		rax, rbx
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
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
L90:
   mov		rax, 0
   add		rsp, 160
   pop		rbp
   ret

; bytes locals   : 32
; bytes temp     : 56
; bytes total    : 128
; (ret_0: *StringBuilder, initialCapacity: int)
NewStringBuilder:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 128
   ; Param ret_0
   mov		-8[rbp], rcx
   ; Param initialCapacity
   mov		-12[rbp], edx
   ; Ln 15: $buffer : *u8 = -24[rbp]
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax
   pop		rcx
   ; (int) 
   call		malloc
   push		rax
   pop		rax
   mov		QWORD -24[rbp], rax
   mov		rax, -24[rbp]
   push		rax
   mov		rax, 0
   push		rax
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*u8, int, int) 
   call		memset
   push		rax
   pop		rax
   ; Return
   lea		rcx, -48[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   mov		rax, -24[rbp]
   push		rax
   pop		rax
   mov		QWORD -48[rbp], rax
   mov		rax, 0
   push		rax
   pop		rax
   mov		DWORD -40[rbp], eax
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -36[rbp], eax
   lea		rax, -48[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   jmp		L92
L92:
   mov		rax, 0
   add		rsp, 128
   pop		rbp
   ret

; bytes locals   : 48
; bytes temp     : 56
; bytes total    : 144
; (sb: *StringBuilder, len: int)
StringBuilder.internalGrow:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 144
   ; Param sb
   mov		-8[rbp], rcx
   ; Param len
   mov		-12[rbp], edx
   ; Ln 27: $newCapacity : int = -16[rbp]
   lea		rax, -8[rbp]
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
   mov		rax, 2
   push		rax
   pop		rbx
   pop		rax
   imul		rax, rbx
   push		rax
   pop		rax
   mov		DWORD -16[rbp], eax
L94:
   mov		eax, DWORD -16[rbp]
   movsx		rax, eax
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
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setle		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L95
   ; While body
   ; Ln 29: Assignment
   mov		eax, DWORD -16[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 2
   push		rax
   pop		rbx
   pop		rax
   imul		rax, rbx
   push		rax
   lea		rax, -16[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
   jmp			L94
L95:
   ; Ln 32: $newBuffer : *u8 = -24[rbp]
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   mov		eax, DWORD -16[rbp]
   movsx		rax, eax
   push		rax
   pop		rdx
   pop		rcx
   ; (*u8, int) 
   call		realloc
   push		rax
   pop		rax
   mov		QWORD -24[rbp], rax
   mov		rax, -24[rbp]
   push		rax
   push		0
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L96
   ; block of if
   ; Ln 34 Print
   ; Pop print arguments
   mov		rcx, CS0
   call		printf
   mov		rax, 1
   push		rax
   pop		rcx
   ; (int) 
   call		exit
   jmp L96
; done
L96:
   ; Ln 38: Assignment
   mov		rax, -24[rbp]
   push		rax
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   ; Ln 39: Assignment
   mov		eax, DWORD -16[rbp]
   movsx		rax, eax
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
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		rax, [rbx]
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
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   mov		rax, 0
   push		rax
   mov		eax, DWORD -16[rbp]
   movsx		rax, eax
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
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*u8, int, int) 
   call		memset
   push		rax
   pop		rax
L93:
   mov		rax, 0
   add		rsp, 144
   pop		rbp
   ret

; bytes locals   : 48
; bytes temp     : 64
; bytes total    : 144
; (sb: *StringBuilder, strPtr: *u8, len: int)
StringBuilder.internalAdd:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 144
   ; Param sb
   mov		-8[rbp], rcx
   ; Param strPtr
   mov		-16[rbp], rdx
   ; Param len
   mov		-20[rbp], r8d
   lea		rax, -8[rbp]
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
   mov		eax, DWORD -20[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
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
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setl		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L99
   ; block of if
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		rax, [rbx]
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
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   mov		rax, -16[rbp]
   push		rax
   mov		eax, DWORD -20[rbp]
   movsx		rax, eax
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*u8, *u8, int) 
   call		memcpy
   push		rax
   pop		rax
   ; Ln 47: Assignment
   lea		rax, -8[rbp]
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
   mov		eax, DWORD -20[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
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
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
   jmp L98
; else
L99:
   mov		rax, -8[rbp]
   push		rax
   mov		eax, DWORD -20[rbp]
   movsx		rax, eax
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, int) 
   call		StringBuilder.internalGrow
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		rax, [rbx]
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
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   mov		rax, -16[rbp]
   push		rax
   mov		eax, DWORD -20[rbp]
   movsx		rax, eax
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*u8, *u8, int) 
   call		memcpy
   push		rax
   pop		rax
   ; Ln 53: Assignment
   lea		rax, -8[rbp]
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
   mov		eax, DWORD -20[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
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
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
   jmp L98
; done
L98:
L97:
   mov		rax, 0
   add		rsp, 144
   pop		rbp
   ret

; bytes locals   : 96
; bytes temp     : 56
; bytes total    : 192
; (sb: *StringBuilder, str: string, start: int, end: int)
StringBuilder.addStringSlice:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 192
   mov		16[rbp], rcx
   mov		24[rbp], rdx
   mov		32[rbp], r8d
   mov		40[rbp], r9d
   ; Param sb
   mov		rax, 16[rbp]
   mov		-8[rbp], rax
   ; Param str
   mov		rax, 24[rbp]
   lea		rbx, -24[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Param start
   mov		rax, 32[rbp]
   mov		-28[rbp], eax
   ; Param end
   mov		rax, 40[rbp]
   mov		-32[rbp], eax
   ; Ln 58: $strLen : int = -36[rbp]
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
   pop		rax
   mov		DWORD -36[rbp], eax
   ; Ln 59: $strPtr : *char = -48[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -48[rbp], rax
   ; Ln 60: Assignment
   mov		rax, -48[rbp]
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
   pop		rax
   mov		QWORD [rbx], rax
   mov		rax, -8[rbp]
   push		rax
   mov		rax, -48[rbp]
   push		rax
   mov		eax, DWORD -36[rbp]
   movsx		rax, eax
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*StringBuilder, *char, int) 
   call		StringBuilder.internalAdd
L100:
   mov		rax, 0
   add		rsp, 192
   pop		rbp
   ret

; bytes locals   : 48
; bytes temp     : 56
; bytes total    : 144
; (sb: *StringBuilder, c: u8)
StringBuilder.addChar:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 144
   ; Param sb
   mov		-8[rbp], rcx
   ; Param c
   mov		-9[rbp], dl
   ; Ln 65: $strLen : int = -16[rbp]
   mov		rax, 1
   push		rax
   pop		rax
   mov		DWORD -16[rbp], eax
   ; Ln 66: $strPtr : *u8 = -24[rbp]
   lea		rax, -9[rbp]
   push		rax
   pop		rax
   mov		QWORD -24[rbp], rax
   mov		rax, -8[rbp]
   push		rax
   mov		rax, -24[rbp]
   push		rax
   mov		eax, DWORD -16[rbp]
   movsx		rax, eax
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*StringBuilder, *u8, int) 
   call		StringBuilder.internalAdd
L101:
   mov		rax, 0
   add		rsp, 144
   pop		rbp
   ret

; bytes locals   : 48
; bytes temp     : 56
; bytes total    : 144
; (sb: *StringBuilder, str: string)
StringBuilder.addString:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 144
   mov		16[rbp], rcx
   mov		24[rbp], rdx
   ; Param sb
   mov		rax, 16[rbp]
   mov		-8[rbp], rax
   ; Param str
   mov		rax, 24[rbp]
   lea		rbx, -24[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   mov		rax, -8[rbp]
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
   ; (*StringBuilder, *char, s64) 
   call		StringBuilder.internalAdd
L102:
   mov		rax, 0
   add		rsp, 144
   pop		rbp
   ret

; bytes locals   : 72
; bytes temp     : 72
; bytes total    : 176
; (sb: *StringBuilder, v: f32)
StringBuilder.addFloat32:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 176
   ; Param sb
   mov		-8[rbp], rcx
   ; Param v
   movd		-12[rbp], xmm1
   ; Ln 75: $buf : [18]u8 = -32[rbp]
   lea		rbx, -32[rbp]
   mov		rcx, rbx
   mov		rdx, 0
   mov		r8, 18
   call		memset
   lea		rax, -32[rbp]
   push		rax
   mov		rax, CS1
   mov		-88[rbp], rax
   mov		QWORD -80[rbp], 2
   lea		rax, -88[rbp]
   push		rax
   pop		rax
   mov	rax, [rax]
   push		rax
   mov		eax, -12[rbp]
   push		rax
   pop		rax
   movd		xmm0, eax
   cvtss2sd	xmm0, xmm0
   movq		rax, xmm0
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*u8, *u8, f32) 
   call		sprintf
   push		rax
   pop		rax
   ; Ln 78: $str : string = -48[rbp]
   mov		QWORD -48[rbp], 0
   mov		QWORD -40[rbp], 0
   ; Ln 79: Assignment
   lea		rax, -32[rbp]
   push		rax
   lea		rax, -48[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   ; Ln 80: Assignment
   lea		rax, -32[rbp]
   push		rax
   pop		rcx
   ; (*u8) 
   call		strlen
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
   mov		rax, -8[rbp]
   push		rax
   lea		rax, -48[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   lea		rax, -48[rbp]
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
   ; (*StringBuilder, *char, s64) 
   call		StringBuilder.internalAdd
L103:
   mov		rax, 0
   add		rsp, 176
   pop		rbp
   ret

; bytes locals   : 72
; bytes temp     : 80
; bytes total    : 192
; (sb: *StringBuilder, v: f64)
StringBuilder.addFloat64:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 192
   ; Param sb
   mov		-8[rbp], rcx
   ; Param v
   movq		-16[rbp], xmm1
   ; Ln 85: $buf : [18]u8 = -40[rbp]
   lea		rbx, -40[rbp]
   mov		rcx, rbx
   mov		rdx, 0
   mov		r8, 18
   call		memset
   lea		rax, -40[rbp]
   push		rax
   mov		rax, CS2
   mov		-88[rbp], rax
   mov		QWORD -80[rbp], 3
   lea		rax, -88[rbp]
   push		rax
   pop		rax
   mov	rax, [rax]
   push		rax
   mov		rax, -16[rbp]
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*u8, *u8, f64) 
   call		sprintf
   push		rax
   pop		rax
   ; Ln 88: $str : string = -56[rbp]
   mov		QWORD -56[rbp], 0
   mov		QWORD -48[rbp], 0
   ; Ln 89: Assignment
   lea		rax, -40[rbp]
   push		rax
   lea		rax, -56[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   ; Ln 90: Assignment
   lea		rax, -40[rbp]
   push		rax
   pop		rcx
   ; (*u8) 
   call		strlen
   push		rax
   lea		rax, -56[rbp]
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
   mov		rax, -8[rbp]
   push		rax
   lea		rax, -56[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   lea		rax, -56[rbp]
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
   ; (*StringBuilder, *char, s64) 
   call		StringBuilder.internalAdd
L104:
   mov		rax, 0
   add		rsp, 192
   pop		rbp
   ret

; bytes locals   : 128
; bytes temp     : 40
; bytes total    : 208
; (sb: *StringBuilder, v: s64)
StringBuilder.addInt64:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 208
   ; Param sb
   mov		-8[rbp], rcx
   ; Param v
   mov		-16[rbp], rdx
   mov		rax, QWORD -16[rbp]
   push		rax
   mov		rax, 0
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L106
   ; block of if
   mov		rax, -8[rbp]
   push		rax
   mov		rax, 48
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, u8) 
   call		StringBuilder.addChar
   ; Return
   jmp		L105
   jmp L106
; done
L106:
   mov		rax, QWORD -16[rbp]
   push		rax
   mov		rax, 0
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setl		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L107
   ; block of if
   mov		rax, -8[rbp]
   push		rax
   mov		rax, 45
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, u8) 
   call		StringBuilder.addChar
   ; Ln 102: Assignment
   mov		rax, QWORD -16[rbp]
   push		rax
   pop		rax
   neg		rax
   push		rax
   lea		rax, -16[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   jmp L107
; done
L107:
   ; Ln 105: $tmp : [32]u8 = -48[rbp]
   lea		rbx, -48[rbp]
   mov		rcx, rbx
   mov		rdx, 0
   mov		r8, 32
   call		memset
   ; Ln 107: $n : int = -52[rbp]
   mov		rax, 0
   push		rax
   pop		rax
   mov		DWORD -52[rbp], eax
L108:
   mov		rax, QWORD -16[rbp]
   push		rax
   mov		rax, 0
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setne		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L109
   ; While body
   ; Ln 109: $digit : s64 = -64[rbp]
   mov		rax, QWORD -16[rbp]
   push		rax
   mov		rax, 10
   push		rax
   pop		rbx
   pop		rax
   cqo
   idiv		rbx
   mov		rax, rdx
   push		rax
   pop		rax
   mov		QWORD -64[rbp], rax
   ; Ln 110: Assignment
   mov		rax, QWORD -16[rbp]
   push		rax
   mov		rax, 10
   push		rax
   pop		rbx
   pop		rax
   cqo
   idiv		rbx
   push		rax
   lea		rax, -16[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   ; Ln 112: $c : u8 = -65[rbp]
   mov		rax, QWORD -64[rbp]
   push		rax
   mov		rax, 48
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   pop		rax
   movzx		eax, al
   push		rax
   pop		rax
   mov		BYTE -65[rbp], al
   ; Ln 114: Assignment
   movzx		eax, BYTE -65[rbp]
   push		rax
   lea		rax, -48[rbp]
   push		rax
   mov		eax, DWORD -52[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   pop		rbx
   pop		rax
   mov		BYTE [rbx], al
   ; Ln 115: Assignment
   mov		eax, DWORD -52[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   lea		rax, -52[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
   jmp			L108
L109:
   ; Ln 119: Ranged for-loop
   mov		rax, 0
   push		rax
   mov		eax, DWORD -52[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD -88[rbp], rax
   mov		QWORD -96[rbp], rbx
   mov		eax, DWORD -88[rbp]
   mov		-80[rbp], eax
L110:
   mov		eax, -96[rbp]
   cmp		-80[rbp], eax
   jge		L112
   ; Ln 120: $ri : int = -100[rbp]
   mov		eax, DWORD -52[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   mov		eax, DWORD -80[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   pop		rax
   mov		DWORD -100[rbp], eax
   ; Ln 121: $c : u8 = -101[rbp]
   lea		rax, -48[rbp]
   push		rax
   mov		eax, DWORD -100[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   imul		rax, 1; Add index expr
   add		rbx, rax
   movzx		eax, BYTE [rbx]
   push		rax
   pop		rax
   mov		BYTE -101[rbp], al
   mov		rax, -8[rbp]
   push		rax
   movzx		eax, BYTE -101[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, u8) 
   call		StringBuilder.addChar
L111:
   inc		DWORD -80[rbp]
   jmp		L110
L112:
L105:
   mov		rax, 0
   add		rsp, 208
   pop		rbp
   ret

; bytes locals   : 128
; bytes temp     : 40
; bytes total    : 208
; (sb: *StringBuilder, v: u64)
StringBuilder.addUInt64:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 208
   ; Param sb
   mov		-8[rbp], rcx
   ; Param v
   mov		-16[rbp], rdx
   mov		rax, QWORD -16[rbp]
   push		rax
   mov		rax, 0
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L114
   ; block of if
   mov		rax, -8[rbp]
   push		rax
   mov		rax, 48
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, u8) 
   call		StringBuilder.addChar
   ; Return
   jmp		L113
   jmp L114
; done
L114:
   ; Ln 132: $tmp : [32]u8 = -48[rbp]
   lea		rbx, -48[rbp]
   mov		rcx, rbx
   mov		rdx, 0
   mov		r8, 32
   call		memset
   ; Ln 134: $n : int = -52[rbp]
   mov		rax, 0
   push		rax
   pop		rax
   mov		DWORD -52[rbp], eax
L115:
   mov		rax, QWORD -16[rbp]
   push		rax
   mov		rax, 0
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setne		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L116
   ; While body
   ; Ln 136: $digit : u64 = -64[rbp]
   mov		rax, QWORD -16[rbp]
   push		rax
   mov		rax, 10
   push		rax
   pop		rbx
   pop		rax
   cqo
   idiv		rbx
   mov		rax, rdx
   push		rax
   pop		rax
   mov		QWORD -64[rbp], rax
   ; Ln 137: Assignment
   mov		rax, QWORD -16[rbp]
   push		rax
   mov		rax, 10
   push		rax
   pop		rbx
   pop		rax
   cqo
   idiv		rbx
   push		rax
   lea		rax, -16[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   ; Ln 139: $c : u8 = -65[rbp]
   mov		rax, QWORD -64[rbp]
   push		rax
   mov		rax, 48
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   pop		rax
   movzx		eax, al
   push		rax
   pop		rax
   mov		BYTE -65[rbp], al
   ; Ln 141: Assignment
   movzx		eax, BYTE -65[rbp]
   push		rax
   lea		rax, -48[rbp]
   push		rax
   mov		eax, DWORD -52[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   pop		rbx
   pop		rax
   mov		BYTE [rbx], al
   ; Ln 142: Assignment
   mov		eax, DWORD -52[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   lea		rax, -52[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
   jmp			L115
L116:
   ; Ln 146: Ranged for-loop
   mov		rax, 0
   push		rax
   mov		eax, DWORD -52[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD -88[rbp], rax
   mov		QWORD -96[rbp], rbx
   mov		eax, DWORD -88[rbp]
   mov		-80[rbp], eax
L117:
   mov		eax, -96[rbp]
   cmp		-80[rbp], eax
   jge		L119
   ; Ln 147: $ri : int = -100[rbp]
   mov		eax, DWORD -52[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   mov		eax, DWORD -80[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   pop		rax
   mov		DWORD -100[rbp], eax
   ; Ln 148: $c : u8 = -101[rbp]
   lea		rax, -48[rbp]
   push		rax
   mov		eax, DWORD -100[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   imul		rax, 1; Add index expr
   add		rbx, rax
   movzx		eax, BYTE [rbx]
   push		rax
   pop		rax
   mov		BYTE -101[rbp], al
   mov		rax, -8[rbp]
   push		rax
   movzx		eax, BYTE -101[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, u8) 
   call		StringBuilder.addChar
L118:
   inc		DWORD -80[rbp]
   jmp		L117
L119:
L113:
   mov		rax, 0
   add		rsp, 208
   pop		rbp
   ret

; bytes locals   : 608
; bytes temp     : 80
; bytes total    : 720
; (sb: *StringBuilder, arg: any)
StringBuilder.addAnyValue:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 720
   mov		16[rbp], rcx
   mov		24[rbp], rdx
   ; Param sb
   mov		rax, 16[rbp]
   mov		-8[rbp], rax
   ; Param arg
   mov		rax, 24[rbp]
   lea		rbx, -24[rbp]
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
   lea		rax, [rbx]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 16
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
   jz			L122
   ; block of if
   ; Ln 155: $prim : *TypePrimitive = -32[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -32[rbp], rax
   lea		rax, -32[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 32
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
   jz			L125
   ; block of if
   ; Ln 158: $value : u8 = -33[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   pop		rax
   mov		BYTE -33[rbp], al
   mov		rax, -8[rbp]
   push		rax
   movzx		eax, BYTE -33[rbp]
   push		rax
   pop		rax
   movzx		rax, al
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, u64) 
   call		StringBuilder.addUInt64
   jmp L124
;#1 else if
L125:
   lea		rax, -32[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 32
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
   jz			L127
   ; Ln 162: $value : u16 = -36[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rbx
   movzx		eax, WORD [rbx]
   push		rax
   pop		rax
   mov		WORD -36[rbp], ax
   mov		rax, -8[rbp]
   push		rax
   movzx		eax, WORD -36[rbp]
   push		rax
   pop		rax
   movzx		rax, ax
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, u64) 
   call		StringBuilder.addUInt64
   jmp L124
;#2 else if
L127:
   lea		rax, -32[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 32
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
   lea		rax, -32[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 32
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   push		rax
   push		0
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
   jz			L128
   ; Ln 166: $value : u32 = -40[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rbx
   mov		eax, DWORD [rbx]
   push		rax
   pop		rax
   mov		DWORD -40[rbp], eax
   mov		rax, -8[rbp]
   push		rax
   mov		eax, DWORD -40[rbp]
   push		rax
   pop		rax
   mov		eax, eax
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, u64) 
   call		StringBuilder.addUInt64
   jmp L124
;#3 else if
L128:
   lea		rax, -32[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 32
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
   jz			L129
   ; Ln 170: $value : u64 = -48[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   pop		rax
   mov		QWORD -48[rbp], rax
   mov		rax, -8[rbp]
   push		rax
   mov		rax, QWORD -48[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, u64) 
   call		StringBuilder.addUInt64
   jmp L124
;#4 else if
L129:
   lea		rax, -32[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 32
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   push		rax
   push		6
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L130
   ; Ln 174: $value : s8 = -49[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rbx
   movzx		eax, BYTE [rbx]
   movsx		rax, al
   push		rax
   pop		rax
   mov		BYTE -49[rbp], al
   mov		rax, -8[rbp]
   push		rax
   movzx		eax, BYTE -49[rbp]
   movsx		rax, al
   push		rax
   pop		rax
   movsx		rax, al
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, s64) 
   call		StringBuilder.addInt64
   jmp L124
;#5 else if
L130:
   lea		rax, -32[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 32
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   push		rax
   push		7
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L131
   ; Ln 178: $value : s16 = -52[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rbx
   movzx		eax, WORD [rbx]
   movsx		rax, ax
   push		rax
   pop		rax
   mov		WORD -52[rbp], ax
   mov		rax, -8[rbp]
   push		rax
   movzx		eax, WORD -52[rbp]
   movsx		rax, ax
   push		rax
   pop		rax
   movsx		rax, ax
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, s64) 
   call		StringBuilder.addInt64
   jmp L124
;#6 else if
L131:
   lea		rax, -32[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 32
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   push		rax
   push		8
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   lea		rax, -32[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 32
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   push		rax
   push		5
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
   jz			L132
   ; Ln 182: $value : s32 = -56[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -56[rbp], eax
   mov		rax, -8[rbp]
   push		rax
   mov		eax, DWORD -56[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   movsxd		rax, eax
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, s64) 
   call		StringBuilder.addInt64
   jmp L124
;#7 else if
L132:
   lea		rax, -32[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 32
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   push		rax
   push		9
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L126
   ; Ln 186: $value : s64 = -64[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   pop		rax
   mov		QWORD -64[rbp], rax
   mov		rax, -8[rbp]
   push		rax
   mov		rax, QWORD -64[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, s64) 
   call		StringBuilder.addInt64
   jmp L124
; else
L126:
   mov		rax, -8[rbp]
   push		rax
   mov		rax, CS3
   mov		-624[rbp], rax
   mov		QWORD -616[rbp], 9
   lea		rax, -624[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   jmp L124
; done
L124:
   jmp L121
;#1 else if
L122:
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 16
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
   jz			L133
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 20
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
   pop		rax
   cmp		al, 0
   jz			L135
   ; block of if
   ; Ln 195: $value : *f32 = -72[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -72[rbp], rax
   mov		rax, -8[rbp]
   push		rax
   mov		rax, -72[rbp]
   push		rax
   pop		rbx
   mov		eax, [rbx]
   push		rax
   pop		rax
   movd		xmm1, eax
   pop		rcx
   ; (*StringBuilder, f32) 
   call		StringBuilder.addFloat32
   jmp L134
; else
L135:
   ; Ln 198: $value : *f64 = -80[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -80[rbp], rax
   mov		rax, -8[rbp]
   push		rax
   mov		rax, -80[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   movq		xmm1, rax
   pop		rcx
   ; (*StringBuilder, f64) 
   call		StringBuilder.addFloat64
   jmp L134
; done
L134:
   jmp L121
;#2 else if
L133:
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 16
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
   jz			L136
   ; Ln 203: $value : *bool = -88[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -88[rbp], rax
   mov		rax, -88[rbp]
   push		rax
   pop		rbx
   movzx		eax, BYTE [rbx]
   push		rax
   pop		rax
   cmp		al, 0
   jz			L138
   ; block of if
   mov		rax, -8[rbp]
   push		rax
   mov		rax, CS4
   mov		-624[rbp], rax
   mov		QWORD -616[rbp], 4
   lea		rax, -624[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   jmp L137
; else
L138:
   mov		rax, -8[rbp]
   push		rax
   mov		rax, CS5
   mov		-624[rbp], rax
   mov		QWORD -616[rbp], 5
   lea		rax, -624[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   jmp L137
; done
L137:
   jmp L121
;#3 else if
L136:
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 16
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
   jz			L139
   ; Ln 211: $value : *string = -96[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -96[rbp], rax
   mov		rax, -8[rbp]
   push		rax
   mov		rax, -96[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   jmp L121
;#4 else if
L139:
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 16
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   push		rax
   push		8
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L140
   ; Ln 217: $enumType : *TypeEnum = -104[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -104[rbp], rax
   mov		rax, -8[rbp]
   push		rax
   mov		rax, CS6
   mov		-624[rbp], rax
   mov		QWORD -616[rbp], 1
   lea		rax, -624[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   mov		rax, -8[rbp]
   push		rax
   lea		rax, -104[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 32
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   jmp L121
;#5 else if
L140:
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 16
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   push		rax
   push		6
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L141
   ; Ln 222: $ptrType : *TypePointer = -112[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -112[rbp], rax
   ; Ln 223: $value : **void = -120[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -120[rbp], rax
   ; Ln 225: $buf : [64]u8 = -184[rbp]
   lea		rbx, -184[rbp]
   mov		rcx, rbx
   mov		rdx, 0
   mov		r8, 64
   call		memset
   ; Ln 226: $written : int = -188[rbp]
   lea		rax, -184[rbp]
   push		rax
   mov		rax, CS7
   mov		-624[rbp], rax
   mov		QWORD -616[rbp], 4
   lea		rax, -624[rbp]
   push		rax
   pop		rax
   mov	rax, [rax]
   push		rax
   mov		rax, -120[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*u8, *u8, *void) 
   call		sprintf
   push		rax
   pop		rax
   mov		DWORD -188[rbp], eax
   ; Ln 228: $str : string = -208[rbp]
   mov		QWORD -208[rbp], 0
   mov		QWORD -200[rbp], 0
   ; Ln 229: Assignment
   lea		rax, -184[rbp]
   push		rax
   lea		rax, -208[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   ; Ln 230: Assignment
   mov		eax, DWORD -188[rbp]
   movsx		rax, eax
   push		rax
   lea		rax, -208[rbp]
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
   mov		rax, -8[rbp]
   push		rax
   lea		rax, -208[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   jmp L121
;#6 else if
L141:
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 16
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   push		rax
   push		5
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L142
   ; Ln 235: $arrType : *TypeArray = -216[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -216[rbp], rax
   ; Ln 236: $data : **void = -224[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -224[rbp], rax
   mov		rax, -224[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   push		0
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L143
   ; block of if
   mov		rax, -8[rbp]
   push		rax
   mov		rax, CS8
   mov		-624[rbp], rax
   mov		QWORD -616[rbp], 2
   lea		rax, -624[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   ; Return
   jmp		L120
   jmp L143
; done
L143:
   mov		rax, -8[rbp]
   push		rax
   mov		rax, CS9
   mov		-624[rbp], rax
   mov		QWORD -616[rbp], 1
   lea		rax, -624[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   ; Ln 244: Ranged for-loop
   mov		rax, 0
   push		rax
   lea		rax, -216[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 48
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD -240[rbp], rax
   mov		QWORD -248[rbp], rbx
   mov		eax, DWORD -240[rbp]
   mov		-232[rbp], eax
L144:
   mov		eax, -248[rbp]
   cmp		-232[rbp], eax
   jge		L146
   ; Ln 246: $elemSize : int = -252[rbp]
   lea		rax, -216[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 40
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 20
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -252[rbp], eax
   ; Ln 248: $elemPtr : *u8 = -264[rbp]
   push		0
   pop		rax
   mov		QWORD -264[rbp], rax
   lea		rax, -216[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 32
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   push		rax
   push		0
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L148
   ; block of if
   ; Ln 250: $arrPtr : **u8 = -272[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -272[rbp], rax
   ; Ln 251: $elem0Ptr : *u8 = -280[rbp]
   mov		rax, -272[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -280[rbp], rax
   ; Ln 252: Assignment
   mov		rax, -280[rbp]
   push		rax
   mov		eax, DWORD -232[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -252[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   imul		rax, rbx
   push		rax
   pop		rax
   pop		rbx
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   lea		rax, -264[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   jmp L147
;#1 else if
L148:
   lea		rax, -216[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 32
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
   jz			L149
   ; Ln 254: $slicePtr : *[]u8 = -288[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -288[rbp], rax
   ; Ln 255: $slice : []u8 = -304[rbp]
   mov		rax, -288[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rax
   lea		rbx, -304[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 256: $elem0 : *u8 = -312[rbp]
   lea		rax, -304[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -312[rbp], rax
   ; Ln 257: Assignment
   mov		rax, -312[rbp]
   push		rax
   mov		eax, DWORD -232[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -252[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   imul		rax, rbx
   push		rax
   pop		rax
   pop		rbx
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   lea		rax, -264[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   jmp L147
;#2 else if
L149:
   lea		rax, -216[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 32
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
   jz			L147
   ; Ln 259: $dynArrPtr : *[..]u8 = -320[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -320[rbp], rax
   ; Ln 260: $dynArr : [..]u8 = -352[rbp]
   mov		rax, -320[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rax
   lea		rbx, -352[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 32
   call		memcpy
   ; Ln 261: $elem0 : *u8 = -360[rbp]
   lea		rax, -352[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -360[rbp], rax
   ; Ln 262: Assignment
   mov		rax, -360[rbp]
   push		rax
   mov		eax, DWORD -232[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -252[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   imul		rax, rbx
   push		rax
   pop		rax
   pop		rbx
   imul		rax, 1; Add index expr
   add		rbx, rax
   push		rbx
   lea		rax, -264[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   jmp L147
; done
L147:
   lea		rax, -609[rbp]
   push		rax
   lea		rax, -216[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 40
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rdx
   pop		rcx
   ; (*Type) 
   call		Type.isPrimitive
   movzx		eax, BYTE -609[rbp]
   push		rax
   lea		rax, -216[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 40
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 16
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

   pop		rbx
   pop		rax
   or 		al, bl
   push		rax
   pop		rax
   cmp		al, 0
   jz			L151
   ; block of if
   ; Ln 268: $value : any = -376[rbp]
   mov		QWORD -376[rbp], 0
   mov		QWORD -368[rbp], 0
   ; Ln 269: Assignment
   lea		rax, -216[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 40
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   lea		rax, -376[rbp]
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
   ; Ln 270: Assignment
   mov		rax, -264[rbp]
   push		rax
   lea		rax, -376[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   mov		rax, -8[rbp]
   push		rax
   lea		rax, -376[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, any) 
   call		StringBuilder.addAnyValue
   jmp L150
; else
L151:
   ; Ln 274: $buf : [64]u8 = -440[rbp]
   lea		rbx, -440[rbp]
   mov		rcx, rbx
   mov		rdx, 0
   mov		r8, 64
   call		memset
   lea		rax, -440[rbp]
   push		rax
   mov		rax, CS10
   mov		-624[rbp], rax
   mov		QWORD -616[rbp], 4
   lea		rax, -624[rbp]
   push		rax
   pop		rax
   mov	rax, [rax]
   push		rax
   mov		rax, -264[rbp]
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*u8, *u8, *u8) 
   call		sprintf
   push		rax
   pop		rax
   ; Ln 276: $str : string = -456[rbp]
   mov		QWORD -456[rbp], 0
   mov		QWORD -448[rbp], 0
   ; Ln 277: Assignment
   lea		rax, -440[rbp]
   push		rax
   lea		rax, -456[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   ; Ln 278: Assignment
   lea		rax, -440[rbp]
   push		rax
   pop		rcx
   ; (*u8) 
   call		strlen
   push		rax
   lea		rax, -456[rbp]
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
   mov		rax, -8[rbp]
   push		rax
   lea		rax, -456[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   jmp L150
; done
L150:
   mov		eax, DWORD -232[rbp]
   movsx		rax, eax
   push		rax
   lea		rax, -216[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 48
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
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setne		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L152
   ; block of if
   mov		rax, -8[rbp]
   push		rax
   mov		rax, CS11
   mov		-624[rbp], rax
   mov		QWORD -616[rbp], 2
   lea		rax, -624[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   jmp L152
; done
L152:
L145:
   inc		DWORD -232[rbp]
   jmp		L144
L146:
   mov		rax, -8[rbp]
   push		rax
   mov		rax, CS12
   mov		-624[rbp], rax
   mov		QWORD -616[rbp], 1
   lea		rax, -624[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   jmp L121
;#7 else if
L142:
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 16
   push		rbx
   pop		rbx
   mov		eax, DWORD [rbx]
   push		rax
   push		7
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L123
   ; Ln 289: $structType : *TypeStruct = -464[rbp]
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		QWORD -464[rbp], rax
   mov		rax, -8[rbp]
   push		rax
   lea		rax, -464[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   mov		rax, -8[rbp]
   push		rax
   mov		rax, CS13
   mov		-624[rbp], rax
   mov		QWORD -616[rbp], 2
   lea		rax, -624[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   ; Ln 292: For-loop
   lea		rax, -464[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 32
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-520[rbp], rbx     ; data
   mov		-528[rbp], rcx     ; count
   mov		QWORD -536[rbp], 0 ; index
L153:
   mov		rbx, -528[rbp]
   mov		rax, -536[rbp]
   cmp		rax, rbx
   jge		L155
   mov		rbx, QWORD -520[rbp]
   mov		rax, QWORD -536[rbp]
   imul		rax, 48
   lea		rbx, [rbx + rax]
   mov		r8, 48
   mov		rdx, rbx
   lea		rcx, -512[rbp]
   call		memcpy
   mov		rax, -8[rbp]
   push		rax
   lea		rax, -512[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   mov		rax, -8[rbp]
   push		rax
   mov		rax, CS14
   mov		-624[rbp], rax
   mov		QWORD -616[rbp], 3
   lea		rax, -624[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   ; Ln 298: $value : any = -552[rbp]
   mov		QWORD -552[rbp], 0
   mov		QWORD -544[rbp], 0
   ; Ln 299: Assignment
   lea		rax, -512[rbp]
   push		rax
   pop		rbx
   add		rbx, 16
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   lea		rax, -552[rbp]
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
   ; Ln 300: Assignment
   lea		rax, -512[rbp]
   push		rax
   pop		rbx
   add		rbx, 24
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   lea		rax, -552[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   mov		rax, -8[rbp]
   push		rax
   lea		rax, -552[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, any) 
   call		StringBuilder.addAnyValue
   mov		eax, DWORD -536[rbp]
   movsx		rax, eax
   push		rax
   lea		rax, -464[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   add		rbx, 32
   push		rbx
   pop		rbx
   lea		rax, [rbx]
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
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setne		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L156
   ; block of if
   mov		rax, -8[rbp]
   push		rax
   mov		rax, CS15
   mov		-624[rbp], rax
   mov		QWORD -616[rbp], 2
   lea		rax, -624[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   jmp L156
; done
L156:
L154:
   inc		QWORD -536[rbp]
   jmp		L153
L155:
   mov		rax, -8[rbp]
   push		rax
   mov		rax, CS16
   mov		-624[rbp], rax
   mov		QWORD -616[rbp], 2
   lea		rax, -624[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   jmp L121
; else
L123:
   mov		rax, -8[rbp]
   push		rax
   mov		rax, CS17
   mov		-624[rbp], rax
   mov		QWORD -616[rbp], 1
   lea		rax, -624[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   mov		rax, -8[rbp]
   push		rax
   lea		rax, -24[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   mov		rax, -8[rbp]
   push		rax
   mov		rax, CS18
   mov		-624[rbp], rax
   mov		QWORD -616[rbp], 1
   lea		rax, -624[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string) 
   call		StringBuilder.addString
   jmp L121
; done
L121:
L120:
   mov		rax, 0
   add		rsp, 720
   pop		rbp
   ret

; bytes locals   : 200
; bytes temp     : 80
; bytes total    : 320
; (sb: *StringBuilder, format: string, args: []any)
StringBuilder.add:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 320
   mov		16[rbp], rcx
   mov		24[rbp], rdx
   mov		32[rbp], r8 
   ; Param sb
   mov		rax, 16[rbp]
   mov		-8[rbp], rax
   ; Param format
   mov		rax, 24[rbp]
   lea		rbx, -24[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Param args
   mov		rax, 32[rbp]
   lea		rbx, -40[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 323: $placeholderIndicies : [..]int = -72[rbp]
   lea		rbx, -72[rbp]
   push		rbx
   mov		rdx, 4
   mov		rcx, 2
   call		calloc
   pop		rbx
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 0
   mov		QWORD 16[rbx], 2
   mov		QWORD 24[rbx], 4
   ; Ln 324: For-loop
   lea		rax, -24[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-88[rbp], rbx     ; data
   mov		-96[rbp], rcx     ; count
   mov		QWORD -104[rbp], 0 ; index
L158:
   mov		rbx, -96[rbp]
   mov		rax, -104[rbp]
   cmp		rax, rbx
   jge		L160
   mov		rbx, QWORD -88[rbp]
   mov		rax, QWORD -104[rbp]
   imul		rax, 1
   lea		rbx, [rbx + rax]
   mov		al, BYTE [rbx]
   mov		-73[rbp], al 
   movzx		eax, BYTE -73[rbp]
   push		rax
   mov		rax, 37
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L161
   ; block of if
   lea		rax, -72[rbp]
   push		rax
   mov		eax, DWORD -104[rbp]
   movsx		rax, eax
   push		rax
   pop		rdx
   pop		rcx
   ; (*[..]int, s32) 
   call		appendInt
   jmp L161
; done
L161:
L159:
   inc		QWORD -104[rbp]
   jmp		L158
L160:
   ; Ln 331: $cursor : int = -108[rbp]
   mov		rax, 0
   push		rax
   pop		rax
   mov		DWORD -108[rbp], eax
   ; Ln 332: For-loop
   lea		rax, -72[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-120[rbp], rbx     ; data
   mov		-128[rbp], rcx     ; count
   mov		QWORD -136[rbp], 0 ; index
L162:
   mov		rbx, -128[rbp]
   mov		rax, -136[rbp]
   cmp		rax, rbx
   jge		L164
   mov		rbx, QWORD -120[rbp]
   mov		rax, QWORD -136[rbp]
   imul		rax, 4
   lea		rbx, [rbx + rax]
   mov		eax, DWORD [rbx]
   mov		-112[rbp], eax 
   mov		rax, -8[rbp]
   push		rax
   lea		rax, -24[rbp]
   push		rax
   mov		eax, DWORD -108[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -112[rbp]
   movsx		rax, eax
   push		rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string, int, int) 
   call		StringBuilder.addStringSlice
   ; Ln 338: $arg : any = -152[rbp]
   lea		rax, -40[rbp]
   push		rax
   mov		eax, DWORD -136[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   mov		rbx, [rbx]
   imul		rax, 16; Add index expr
   add		rbx, rax
   lea		rax, [rbx]
   push		rax
   pop		rax
   lea		rbx, -152[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   mov		rax, -8[rbp]
   push		rax
   lea		rax, -152[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, any) 
   call		StringBuilder.addAnyValue
   ; Ln 341: Assignment
   mov		eax, DWORD -112[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   lea		rax, -108[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
L163:
   inc		QWORD -136[rbp]
   jmp		L162
L164:
   mov		rax, -8[rbp]
   push		rax
   lea		rax, -24[rbp]
   push		rax
   mov		eax, DWORD -108[rbp]
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
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string, int, s64) 
   call		StringBuilder.addStringSlice
L157:
   mov		rax, 0
   add		rsp, 320
   pop		rbp
   ret

; bytes locals   : 16
; bytes temp     : 56
; bytes total    : 112
; (sb: *StringBuilder)
StringBuilder.clear:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 112
   ; Param sb
   mov		-8[rbp], rcx
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   mov		rax, 0
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
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*u8, int, int) 
   call		memset
   push		rax
   pop		rax
L165:
   mov		rax, 0
   add		rsp, 112
   pop		rbp
   ret

; bytes locals   : 40
; bytes temp     : 0
; bytes total    : 80
; (ret_0: *string, sb: *StringBuilder)
StringBuilder.toString:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 80
   ; Param ret_0
   mov		-8[rbp], rcx
   ; Param sb
   mov		-16[rbp], rdx
   ; Ln 353: $str : string = -32[rbp]
   mov		QWORD -32[rbp], 0
   mov		QWORD -24[rbp], 0
   ; Ln 354: Assignment
   lea		rax, -16[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   lea		rax, -32[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD [rbx], rax
   ; Ln 355: Assignment
   lea		rax, -16[rbp]
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
   lea		rax, -32[rbp]
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
   lea		rax, -32[rbp]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   jmp		L166
L166:
   mov		rax, 0
   add		rsp, 80
   pop		rbp
   ret

; bytes locals   : 16
; bytes temp     : 24
; bytes total    : 80
; (sb: *StringBuilder)
StringBuilder.free:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 80
   ; Param sb
   mov		-8[rbp], rcx
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rcx
   ; (*u8) 
   call		free
L167:
   mov		rax, 0
   add		rsp, 80
   pop		rbp
   ret

; bytes locals   : 232
; bytes temp     : 80
; bytes total    : 352
; (format: string, args: []any, newline: bool)
Print:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 352
   mov		16[rbp], rcx
   mov		24[rbp], rdx
   mov		32[rbp], r8b
   ; Param format
   mov		rax, 16[rbp]
   lea		rbx, -16[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Param args
   mov		rax, 24[rbp]
   lea		rbx, -32[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Param newline
   mov		rax, 32[rbp]
   mov		-33[rbp], al
   ; Ln 15: $placeholderIndicies : [..]int = -72[rbp]
   lea		rbx, -72[rbp]
   push		rbx
   mov		rdx, 4
   mov		rcx, 2
   call		calloc
   pop		rbx
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 0
   mov		QWORD 16[rbx], 2
   mov		QWORD 24[rbx], 4
   ; Ln 16: For-loop
   lea		rax, -16[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-88[rbp], rbx     ; data
   mov		-96[rbp], rcx     ; count
   mov		QWORD -104[rbp], 0 ; index
L169:
   mov		rbx, -96[rbp]
   mov		rax, -104[rbp]
   cmp		rax, rbx
   jge		L171
   mov		rbx, QWORD -88[rbp]
   mov		rax, QWORD -104[rbp]
   imul		rax, 1
   lea		rbx, [rbx + rax]
   mov		al, BYTE [rbx]
   mov		-73[rbp], al 
   movzx		eax, BYTE -73[rbp]
   push		rax
   push		37
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L172
   ; block of if
   lea		rax, -72[rbp]
   push		rax
   mov		eax, DWORD -104[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   movsxd		rax, eax
   push		rax
   pop		rdx
   pop		rcx
   ; (*[..]int, s64) 
   call		appendInt
   jmp L172
; done
L172:
L170:
   inc		QWORD -104[rbp]
   jmp		L169
L171:
   ; Ln 23: $sb : StringBuilder = -120[rbp]
   lea		rax, -248[rbp]
   push		rax
   mov		rax, 1024
   push		rax
   pop		rdx
   pop		rcx
   ; (int) 
   call		NewStringBuilder
   lea		rax, -248[rbp]
   push		rax
   pop		rax
   lea		rbx, -120[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 25: $cursor : int = -124[rbp]
   mov		rax, 0
   push		rax
   pop		rax
   mov		DWORD -124[rbp], eax
   ; Ln 26: For-loop
   lea		rax, -72[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-136[rbp], rbx     ; data
   mov		-144[rbp], rcx     ; count
   mov		QWORD -152[rbp], 0 ; index
L173:
   mov		rbx, -144[rbp]
   mov		rax, -152[rbp]
   cmp		rax, rbx
   jge		L175
   mov		rbx, QWORD -136[rbp]
   mov		rax, QWORD -152[rbp]
   imul		rax, 4
   lea		rbx, [rbx + rax]
   mov		eax, DWORD [rbx]
   mov		-128[rbp], eax 
   lea		rax, -120[rbp]
   push		rax
   lea		rax, -16[rbp]
   push		rax
   mov		eax, DWORD -124[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -128[rbp]
   movsx		rax, eax
   push		rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string, int, int) 
   call		StringBuilder.addStringSlice
   ; Ln 32: $arg : any = -168[rbp]
   lea		rax, -32[rbp]
   push		rax
   mov		eax, DWORD -152[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   mov		rbx, [rbx]
   imul		rax, 16; Add index expr
   add		rbx, rax
   lea		rax, [rbx]
   push		rax
   pop		rax
   lea		rbx, -168[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -120[rbp]
   push		rax
   lea		rax, -168[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder, any) 
   call		StringBuilder.addAnyValue
   ; Ln 35: Assignment
   mov		eax, DWORD -128[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   lea		rax, -124[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
L174:
   inc		QWORD -152[rbp]
   jmp		L173
L175:
   lea		rax, -120[rbp]
   push		rax
   lea		rax, -16[rbp]
   push		rax
   mov		eax, DWORD -124[rbp]
   movsx		rax, eax
   push		rax
   lea		rax, -16[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, QWORD [rbx]
   push		rax
   pop		r9 
   pop		r8 
   pop		rdx
   pop		rcx
   ; (*StringBuilder, string, int, s64) 
   call		StringBuilder.addStringSlice
   ; Ln 41: $result : string = -184[rbp]
   lea		rax, -248[rbp]
   push		rax
   lea		rax, -120[rbp]
   push		rax
   pop		rdx
   pop		rcx
   ; (*StringBuilder) 
   call		StringBuilder.toString
   lea		rax, -248[rbp]
   push		rax
   pop		rax
   lea		rbx, -184[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   movzx		eax, BYTE -33[rbp]
   push		rax
   pop		rax
   cmp		al, 0
   jz			L177
   ; block of if
   mov		rax, CS19
   mov		-248[rbp], rax
   mov		QWORD -240[rbp], 4
   lea		rax, -248[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   lea		rax, -184[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rdx
   pop		rcx
   ; (*char, *char) 
   call		printf
   push		rax
   pop		rax
   jmp L176
; else
L177:
   mov		rax, CS20
   mov		-248[rbp], rax
   mov		QWORD -240[rbp], 2
   lea		rax, -248[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   lea		rax, -184[rbp]
   push		rax
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rdx
   pop		rcx
   ; (*char, *char) 
   call		printf
   push		rax
   pop		rax
   jmp L176
; done
L176:
   lea		rax, -120[rbp]
   push		rax
   pop		rcx
   ; (*StringBuilder) 
   call		StringBuilder.free
   ; Return
   jmp		L168
L168:
   mov		rax, 0
   add		rsp, 352
   pop		rbp
   ret

; bytes locals   : 320
; bytes temp     : 200
; bytes total    : 560
; ()
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 560
   mov		rax, CS25
   mov		-336[rbp], rax
   mov		QWORD -328[rbp], 1
   lea		rax, -336[rbp]
   push		rax
   mov		eax, [Red.22]
   push		rax
   pop		rax
   mov		-20[rbp], eax
   lea		rax, -20[rbp]
   lea		rbx, -352[rbp]
   mov		QWORD 0[rbx], rax
   mov		QWORD rbx, -352[rbp]
   mov		-360[rbp], rbx
   sub		rsp, 32
   mov		rcx, 192
   call		malloc
   add		rsp, 32
   mov		-368[rbp], rax
   mov		-376[rbp], rax
   mov		rbx, -360[rbp]
   lea		rbx, 0[rbx]
   mov		rax, QWORD [Type_u8]
   push		rax
   pop		rcx
   mov		rdx, -360[rbp]
   lea		rdx, 0[rdx]
   mov		rbx, -376[rbp]
   mov		rax, CS26
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 1
   mov		QWORD 16[rbx], rcx
   mov		QWORD 24[rbx], rdx
   mov		DWORD 32[rbx], 0
   mov		DWORD 36[rbx], 0
   add		rbx, 48
   mov		-376[rbp], rbx
   mov		rbx, -360[rbp]
   lea		rbx, 1[rbx]
   mov		rax, QWORD [Type_u8]
   push		rax
   pop		rcx
   mov		rdx, -360[rbp]
   lea		rdx, 1[rdx]
   mov		rbx, -376[rbp]
   mov		rax, CS27
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 1
   mov		QWORD 16[rbx], rcx
   mov		QWORD 24[rbx], rdx
   mov		DWORD 32[rbx], 1
   mov		DWORD 36[rbx], 1
   add		rbx, 48
   mov		-376[rbp], rbx
   mov		rbx, -360[rbp]
   lea		rbx, 2[rbx]
   mov		rax, QWORD [Type_u8]
   push		rax
   pop		rcx
   mov		rdx, -360[rbp]
   lea		rdx, 2[rdx]
   mov		rbx, -376[rbp]
   mov		rax, CS28
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 1
   mov		QWORD 16[rbx], rcx
   mov		QWORD 24[rbx], rdx
   mov		DWORD 32[rbx], 2
   mov		DWORD 36[rbx], 2
   add		rbx, 48
   mov		-376[rbp], rbx
   mov		rbx, -360[rbp]
   lea		rbx, 3[rbx]
   mov		rax, QWORD [Type_u8]
   push		rax
   pop		rcx
   mov		rdx, -360[rbp]
   lea		rdx, 3[rdx]
   mov		rbx, -376[rbp]
   mov		rax, CS29
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 1
   mov		QWORD 16[rbx], rcx
   mov		QWORD 24[rbx], rdx
   mov		DWORD 32[rbx], 3
   mov		DWORD 36[rbx], 3
   add		rbx, 48
   mov		-376[rbp], rbx
   mov		rax, CS30
   mov		QWORD -392[rbp], rax
   mov		QWORD -384[rbp], 5
   lea		rcx, -392[rbp]
   mov		rdx, -368[rbp]
   mov		r8d, 4
   mov		r9d, 4
   sub		rsp, 40
   mov		DWORD 32[rsp], 1
   call		runtime_get_type_struct
   add		rsp, 40
   push		rax
   pop		rcx
   lea		rbx, -352[rbp]
   mov		QWORD 8[rbx], rcx
   push		rbx
   pop		rax
   lea		rbx, -16[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -16[rbp]
   mov		QWORD -408[rbp], rax
   mov		QWORD -400[rbp], 1
   lea		rax, -408[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		QWORD -424[rbp], rbx
   mov		QWORD -416[rbp], rcx
   lea		rax, -424[rbp]
   push		rax
   push		1
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, []any, bool) 
   call		Print
   mov		rax, CS31
   mov		-336[rbp], rax
   mov		QWORD -328[rbp], 1
   lea		rax, -336[rbp]
   push		rax
   mov		eax, [Green.23]
   push		rax
   pop		rax
   mov		-44[rbp], eax
   lea		rax, -44[rbp]
   lea		rbx, -352[rbp]
   mov		QWORD 0[rbx], rax
   mov		QWORD rbx, -352[rbp]
   mov		-360[rbp], rbx
   sub		rsp, 32
   mov		rcx, 192
   call		malloc
   add		rsp, 32
   mov		-368[rbp], rax
   mov		-376[rbp], rax
   mov		rbx, -360[rbp]
   lea		rbx, 0[rbx]
   mov		rax, QWORD [Type_u8]
   push		rax
   pop		rcx
   mov		rdx, -360[rbp]
   lea		rdx, 0[rdx]
   mov		rbx, -376[rbp]
   mov		rax, CS32
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 1
   mov		QWORD 16[rbx], rcx
   mov		QWORD 24[rbx], rdx
   mov		DWORD 32[rbx], 0
   mov		DWORD 36[rbx], 0
   add		rbx, 48
   mov		-376[rbp], rbx
   mov		rbx, -360[rbp]
   lea		rbx, 1[rbx]
   mov		rax, QWORD [Type_u8]
   push		rax
   pop		rcx
   mov		rdx, -360[rbp]
   lea		rdx, 1[rdx]
   mov		rbx, -376[rbp]
   mov		rax, CS33
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 1
   mov		QWORD 16[rbx], rcx
   mov		QWORD 24[rbx], rdx
   mov		DWORD 32[rbx], 1
   mov		DWORD 36[rbx], 1
   add		rbx, 48
   mov		-376[rbp], rbx
   mov		rbx, -360[rbp]
   lea		rbx, 2[rbx]
   mov		rax, QWORD [Type_u8]
   push		rax
   pop		rcx
   mov		rdx, -360[rbp]
   lea		rdx, 2[rdx]
   mov		rbx, -376[rbp]
   mov		rax, CS34
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 1
   mov		QWORD 16[rbx], rcx
   mov		QWORD 24[rbx], rdx
   mov		DWORD 32[rbx], 2
   mov		DWORD 36[rbx], 2
   add		rbx, 48
   mov		-376[rbp], rbx
   mov		rbx, -360[rbp]
   lea		rbx, 3[rbx]
   mov		rax, QWORD [Type_u8]
   push		rax
   pop		rcx
   mov		rdx, -360[rbp]
   lea		rdx, 3[rdx]
   mov		rbx, -376[rbp]
   mov		rax, CS35
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 1
   mov		QWORD 16[rbx], rcx
   mov		QWORD 24[rbx], rdx
   mov		DWORD 32[rbx], 3
   mov		DWORD 36[rbx], 3
   add		rbx, 48
   mov		-376[rbp], rbx
   mov		rax, CS36
   mov		QWORD -392[rbp], rax
   mov		QWORD -384[rbp], 5
   lea		rcx, -392[rbp]
   mov		rdx, -368[rbp]
   mov		r8d, 4
   mov		r9d, 4
   sub		rsp, 40
   mov		DWORD 32[rsp], 1
   call		runtime_get_type_struct
   add		rsp, 40
   push		rax
   pop		rcx
   lea		rbx, -352[rbp]
   mov		QWORD 8[rbx], rcx
   push		rbx
   pop		rax
   lea		rbx, -40[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -40[rbp]
   mov		QWORD -408[rbp], rax
   mov		QWORD -400[rbp], 1
   lea		rax, -408[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		QWORD -424[rbp], rbx
   mov		QWORD -416[rbp], rcx
   lea		rax, -424[rbp]
   push		rax
   push		1
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, []any, bool) 
   call		Print
   mov		rax, CS37
   mov		-336[rbp], rax
   mov		QWORD -328[rbp], 1
   lea		rax, -336[rbp]
   push		rax
   mov		eax, [Blue.24]
   push		rax
   pop		rax
   mov		-68[rbp], eax
   lea		rax, -68[rbp]
   lea		rbx, -352[rbp]
   mov		QWORD 0[rbx], rax
   mov		QWORD rbx, -352[rbp]
   mov		-360[rbp], rbx
   sub		rsp, 32
   mov		rcx, 192
   call		malloc
   add		rsp, 32
   mov		-368[rbp], rax
   mov		-376[rbp], rax
   mov		rbx, -360[rbp]
   lea		rbx, 0[rbx]
   mov		rax, QWORD [Type_u8]
   push		rax
   pop		rcx
   mov		rdx, -360[rbp]
   lea		rdx, 0[rdx]
   mov		rbx, -376[rbp]
   mov		rax, CS38
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 1
   mov		QWORD 16[rbx], rcx
   mov		QWORD 24[rbx], rdx
   mov		DWORD 32[rbx], 0
   mov		DWORD 36[rbx], 0
   add		rbx, 48
   mov		-376[rbp], rbx
   mov		rbx, -360[rbp]
   lea		rbx, 1[rbx]
   mov		rax, QWORD [Type_u8]
   push		rax
   pop		rcx
   mov		rdx, -360[rbp]
   lea		rdx, 1[rdx]
   mov		rbx, -376[rbp]
   mov		rax, CS39
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 1
   mov		QWORD 16[rbx], rcx
   mov		QWORD 24[rbx], rdx
   mov		DWORD 32[rbx], 1
   mov		DWORD 36[rbx], 1
   add		rbx, 48
   mov		-376[rbp], rbx
   mov		rbx, -360[rbp]
   lea		rbx, 2[rbx]
   mov		rax, QWORD [Type_u8]
   push		rax
   pop		rcx
   mov		rdx, -360[rbp]
   lea		rdx, 2[rdx]
   mov		rbx, -376[rbp]
   mov		rax, CS40
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 1
   mov		QWORD 16[rbx], rcx
   mov		QWORD 24[rbx], rdx
   mov		DWORD 32[rbx], 2
   mov		DWORD 36[rbx], 2
   add		rbx, 48
   mov		-376[rbp], rbx
   mov		rbx, -360[rbp]
   lea		rbx, 3[rbx]
   mov		rax, QWORD [Type_u8]
   push		rax
   pop		rcx
   mov		rdx, -360[rbp]
   lea		rdx, 3[rdx]
   mov		rbx, -376[rbp]
   mov		rax, CS41
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 1
   mov		QWORD 16[rbx], rcx
   mov		QWORD 24[rbx], rdx
   mov		DWORD 32[rbx], 3
   mov		DWORD 36[rbx], 3
   add		rbx, 48
   mov		-376[rbp], rbx
   mov		rax, CS42
   mov		QWORD -392[rbp], rax
   mov		QWORD -384[rbp], 5
   lea		rcx, -392[rbp]
   mov		rdx, -368[rbp]
   mov		r8d, 4
   mov		r9d, 4
   sub		rsp, 40
   mov		DWORD 32[rsp], 1
   call		runtime_get_type_struct
   add		rsp, 40
   push		rax
   pop		rcx
   lea		rbx, -352[rbp]
   mov		QWORD 8[rbx], rcx
   push		rbx
   pop		rax
   lea		rbx, -64[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -64[rbp]
   mov		QWORD -408[rbp], rax
   mov		QWORD -400[rbp], 1
   lea		rax, -408[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		QWORD -424[rbp], rbx
   mov		QWORD -416[rbp], rcx
   lea		rax, -424[rbp]
   push		rax
   push		1
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, []any, bool) 
   call		Print
   ; Ln 18: For-loop
   lea		rax, [Rainbow.25]
   push		rax
   pop		rax
   mov		rbx, rax
   mov		rcx, 3
   mov		-80[rbp], rbx     ; data
   mov		-88[rbp], rcx     ; count
   mov		QWORD -96[rbp], 0 ; index
L179:
   mov		rbx, -88[rbp]
   mov		rax, -96[rbp]
   cmp		rax, rbx
   jge		L181
   mov		rbx, QWORD -80[rbp]
   mov		rax, QWORD -96[rbp]
   imul		rax, 4
   lea		rbx, [rbx + rax]
   mov		eax, DWORD [rbx]
   mov		-72[rbp], eax 
   mov		rax, CS43
   mov		-336[rbp], rax
   mov		QWORD -328[rbp], 1
   lea		rax, -336[rbp]
   push		rax
   mov		eax, -72[rbp]
   push		rax
   pop		rax
   mov		-116[rbp], eax
   lea		rax, -116[rbp]
   lea		rbx, -352[rbp]
   mov		QWORD 0[rbx], rax
   mov		QWORD rbx, -352[rbp]
   mov		-360[rbp], rbx
   sub		rsp, 32
   mov		rcx, 192
   call		malloc
   add		rsp, 32
   mov		-368[rbp], rax
   mov		-376[rbp], rax
   mov		rbx, -360[rbp]
   lea		rbx, 0[rbx]
   mov		rax, QWORD [Type_u8]
   push		rax
   pop		rcx
   mov		rdx, -360[rbp]
   lea		rdx, 0[rdx]
   mov		rbx, -376[rbp]
   mov		rax, CS44
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 1
   mov		QWORD 16[rbx], rcx
   mov		QWORD 24[rbx], rdx
   mov		DWORD 32[rbx], 0
   mov		DWORD 36[rbx], 0
   add		rbx, 48
   mov		-376[rbp], rbx
   mov		rbx, -360[rbp]
   lea		rbx, 1[rbx]
   mov		rax, QWORD [Type_u8]
   push		rax
   pop		rcx
   mov		rdx, -360[rbp]
   lea		rdx, 1[rdx]
   mov		rbx, -376[rbp]
   mov		rax, CS45
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 1
   mov		QWORD 16[rbx], rcx
   mov		QWORD 24[rbx], rdx
   mov		DWORD 32[rbx], 1
   mov		DWORD 36[rbx], 1
   add		rbx, 48
   mov		-376[rbp], rbx
   mov		rbx, -360[rbp]
   lea		rbx, 2[rbx]
   mov		rax, QWORD [Type_u8]
   push		rax
   pop		rcx
   mov		rdx, -360[rbp]
   lea		rdx, 2[rdx]
   mov		rbx, -376[rbp]
   mov		rax, CS46
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 1
   mov		QWORD 16[rbx], rcx
   mov		QWORD 24[rbx], rdx
   mov		DWORD 32[rbx], 2
   mov		DWORD 36[rbx], 2
   add		rbx, 48
   mov		-376[rbp], rbx
   mov		rbx, -360[rbp]
   lea		rbx, 3[rbx]
   mov		rax, QWORD [Type_u8]
   push		rax
   pop		rcx
   mov		rdx, -360[rbp]
   lea		rdx, 3[rdx]
   mov		rbx, -376[rbp]
   mov		rax, CS47
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 1
   mov		QWORD 16[rbx], rcx
   mov		QWORD 24[rbx], rdx
   mov		DWORD 32[rbx], 3
   mov		DWORD 36[rbx], 3
   add		rbx, 48
   mov		-376[rbp], rbx
   mov		rax, CS48
   mov		QWORD -392[rbp], rax
   mov		QWORD -384[rbp], 5
   lea		rcx, -392[rbp]
   mov		rdx, -368[rbp]
   mov		r8d, 4
   mov		r9d, 4
   sub		rsp, 40
   mov		DWORD 32[rsp], 1
   call		runtime_get_type_struct
   add		rsp, 40
   push		rax
   pop		rcx
   lea		rbx, -352[rbp]
   mov		QWORD 8[rbx], rcx
   push		rbx
   pop		rax
   lea		rbx, -112[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -112[rbp]
   mov		QWORD -408[rbp], rax
   mov		QWORD -400[rbp], 1
   lea		rax, -408[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		QWORD -424[rbp], rbx
   mov		QWORD -416[rbp], rcx
   lea		rax, -424[rbp]
   push		rax
   push		1
   pop		r8 
   pop		rdx
   pop		rcx
   ; (string, []any, bool) 
   call		Print
L180:
   inc		QWORD -96[rbp]
   jmp		L179
L181:
L178:
   mov		rax, 0
   add		rsp, 560
   pop		rbp
   ret
