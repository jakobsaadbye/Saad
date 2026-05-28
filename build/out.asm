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
   CS0 DB `%d`, 0 
   CS2 DB `%s`, 0 
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
   C_2.data DB "Hello world", 0
   align 8
   C_2:
   dq C_2.data
   dq 11
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
   extern calloc
   extern realloc
   extern free
   extern exit
   extern strlen
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
; (ret_0: *bool, type: *Type) -> *bool
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
; (ret_0: **u8, s: string) -> **u8
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
; (ret_0: *string, cstr: *u8) -> *string
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
; (ret_0: *string, s: string, from: int, to: int) -> *string
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
; (ret_0: *string, s: string) -> *string
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
; (ret_0: *string, s: string) -> *string
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
; (ret_0: *string, s: string) -> *string
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
; (ret_0: *bool, s: string, str: string) -> *bool
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
; (ret_0: *bool, s: string, str: string) -> *bool
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
; (ret_0: *int, s: string, str: string, fromIndex: int) -> *int
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
; (ret_0: *[..]string, s: string, sep: string) -> *[..]string
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
; (ret_0: *string, s: string) -> *string
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

; bytes locals   : 8
; bytes temp     : 24
; bytes total    : 64
; ()
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64
   ; Ln 2: $myInt : int = -4[rbp]
   mov		rax, 10
   push		rax
   pop		rax
   mov		DWORD -4[rbp], eax
   ; () 
   call		a
   ; Ln 10 Print
   mov		eax, DWORD -4[rbp]
   movsx		rax, eax
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS0
   call		printf
L90:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret

; bytes locals   : 0
; bytes temp     : 24
; bytes total    : 64
; ()
a:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64
   ; Ln 5 Print
   mov		rax, C_2
   push		rax
   pop		rbx
   mov		-8[rbp], rbx
   mov		rdx, 8[rbx]
   add		rdx, 1
   mov		rcx, 1
   sub		rsp, 32
   call		calloc
   add		rsp, 32
   mov		rbx, -8[rbp]
   mov		rdx, 0[rbx]
   mov		r8d, 8[rbx]
   mov		-8[rbp], rax
   mov		rcx, rax
   sub		rsp, 32
   call		memcpy
   add		rsp, 32
   mov		rax, -8[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS2
   call		printf
L91:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret

