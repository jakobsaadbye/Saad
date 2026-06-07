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

; bytes locals   : 32
; bytes temp     : 0
; bytes total    : 64
; ()
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64
   ; Ln 2: $a : int = -4[rbp]
   mov		rax, 3
   push		rax
   mov		rax, 5
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   pop		rax
   mov		DWORD -4[rbp], eax
   ; Ln 3: $c : int = -8[rbp]
   mov		eax, DWORD -4[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 10
   push		rax
   pop		rbx
   pop		rax
   imul		rax, rbx
   push		rax
   pop		rax
   mov		DWORD -8[rbp], eax
   ; Ln 4: $d : int = -12[rbp]
   mov		eax, DWORD -4[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -8[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   pop		rax
   mov		DWORD -12[rbp], eax
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 10
   push		rax
   pop		rbx
   pop		rax
   cmp		rax, rbx
   setl		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L43
   ; block of if
   ; Ln 6: $e : int = -16[rbp]
   mov		rax, 2
   push		rax
   pop		rax
   mov		DWORD -16[rbp], eax
   jmp L42
; else
L43:
   ; Ln 8: Assignment
   mov		rax, 5
   push		rax
   lea		rax, -4[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
   jmp L42
; done
L42:
   ; Ln 10: Assignment
   mov		rax, 69
   push		rax
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
L41:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret

