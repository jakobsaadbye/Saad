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
   __TypeKind.Void DB ".Void", 0
   __TypeKind.Bool DB ".Bool", 0
   __TypeKind.Int DB ".Int", 0
   __TypeKind.Float DB ".Float", 0
   __TypeKind.String DB ".String", 0
   __TypeKind.Array DB ".Array", 0
   __TypeKind.Pointer DB ".Pointer", 0
   __TypeKind.Struct DB ".Struct", 0
   __TypeKind.Enum DB ".Enum", 0
   __TypeKind.Function DB ".Function", 0
   __TypeKind.Tuple DB ".Tuple", 0
   __TypeKind.Variadic DB ".Variadic", 0
   __TypeKind.Any DB ".Any", 0
   __TypePrimitiveKind.Uint DB ".Uint", 0
   __TypePrimitiveKind.U8 DB ".U8", 0
   __TypePrimitiveKind.U16 DB ".U16", 0
   __TypePrimitiveKind.U32 DB ".U32", 0
   __TypePrimitiveKind.U64 DB ".U64", 0
   __TypePrimitiveKind.Int DB ".Int", 0
   __TypePrimitiveKind.S8 DB ".S8", 0
   __TypePrimitiveKind.S16 DB ".S16", 0
   __TypePrimitiveKind.S32 DB ".S32", 0
   __TypePrimitiveKind.S64 DB ".S64", 0
   __TypePrimitiveKind.Float DB ".Float", 0
   __TypePrimitiveKind.F32 DB ".F32", 0
   __TypePrimitiveKind.F64 DB ".F64", 0
   __TypePrimitiveKind.String DB ".String", 0
   __TypePrimitiveKind.Bool DB ".Bool", 0
   __TypePrimitiveKind.Void DB ".Void", 0
   __TypePrimitiveKind.UntypedInt DB ".UntypedInt", 0
   __TypePrimitiveKind.UntypedFloat DB ".UntypedFloat", 0
   __TypeArrayKind.Fixed DB ".Fixed", 0
   __TypeArrayKind.Slice DB ".Slice", 0
   __TypeArrayKind.Dynamic DB ".Dynamic", 0
   CS0 DB `OutOfMemoryException: Failed to reallocate a new buffer for the string builder`, 0 
   CS1 DB `x, y, z = %d %d %d`, 0 
   enum_buffer_0 times 20 DB 0
   CS2 DB `arg.type = Type{ kind = %s, name = %s, size = %d, flags = %d }\n`, 0 
   CS3 DB `Format string = %s\n`, 0 
   CS4 DB `%llu `, 0 
   CS5 DB `arg.type = Type{ kind = %s, name = %s, size = %d, flags = %d }\n`, 0 
   __HttpStatus.Ok DB ".Ok", 0
   __HttpStatus.Created DB ".Created", 0
   __HttpStatus.BadRequest DB ".BadRequest", 0
   CS6 DB `% % % % % % Hellope`, 0 
   CF7 DD 3.1400000
   CS8 DB `Hellope`, 0 
   CS9 DB `HttpStatus`, 0 
   CF10 DD 1.0000000
   CF11 DD 4.0000000
   CF12 DD 9.0000000
   CS13 DB `x`, 0 
   CS14 DB `y`, 0 
   CS15 DB `z`, 0 
   CS16 DB `Vector3`, 0 
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
   extern runtime_get_type_array
   extern runtime_get_type_enum
   extern runtime_get_type_struct
   extern runtime_get_type_any
   extern strlen
   extern runtime_get_type_array
   extern runtime_get_type_enum
   extern runtime_get_type_struct
   extern runtime_get_type_any
   extern memset
   extern memcpy
   extern malloc
   extern realloc
   extern free
   extern exit
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

; bytes locals   : 32
; bytes temp     : 56
; bytes total    : 128
NewStringBuilder:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 128
   ; Param ret_0
   mov		-8[rbp], rcx
   ; Param initialCapacity
   mov		-12[rbp], edx
   ; Ln 10: $buffer : *u8 = -24[rbp]
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax
   pop		rcx
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
   jmp		L37
L37:
   mov		rax, 0
   add		rsp, 128
   pop		rbp
   ret

; bytes locals   : 96
; bytes temp     : 72
; bytes total    : 208
StringBuilder.append:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 208
   mov		16[rbp], rcx
   mov		24[rbp], rdx
   mov		32[rbp], r8 
   ; Param sb
   mov		rax, 16[rbp]
   mov		-8[rbp], rax
   ; Param format
   mov		rax, 24[rbp]
   mov		-16[rbp], rax
   ; Param args
   mov		rax, 32[rbp]
   lea		rbx, -32[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 24: $str : *u8 = -40[rbp]
   push		2048
   pop		rcx
   call		malloc
   push		rax
   pop		rax
   mov		QWORD -40[rbp], rax
   ; Ln 25: $strLen : int = -44[rbp]
   mov		rax, -40[rbp]
   push		rax
   mov		rax, QWORD -16[rbp]
   push		rax
   lea		rax, -32[rbp]
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   call		sprintf
   push		rax
   pop		rax
   mov		DWORD -44[rbp], eax
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
   mov		eax, DWORD -44[rbp]
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
   jz			L40
   ; block of if
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   lea		rax, [rbx]
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
   mov		rax, -40[rbp]
   push		rax
   mov		eax, DWORD -44[rbp]
   movsx		rax, eax
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   call		memcpy
   push		rax
   pop		rax
   ; Ln 29: Assignment
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
   mov		eax, DWORD -44[rbp]
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
   mov		rax, -40[rbp]
   push		rax
   pop		rcx
   call		free
   jmp L39
; else
L40:
   ; Ln 32: $newCapacity : int = -48[rbp]
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
   mov		DWORD -48[rbp], eax
L41:
   mov		eax, DWORD -48[rbp]
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
   mov		eax, DWORD -44[rbp]
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
   jz			L42
   ; While body
   ; Ln 34: Assignment
   mov		eax, DWORD -48[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 2
   push		rax
   pop		rbx
   pop		rax
   imul		rax, rbx
   push		rax
   lea		rax, -48[rbp]
   push		rax
   pop		rbx
   pop		rax
   mov		DWORD [rbx], eax
   jmp			L41
L42:
   ; Ln 37: $newBuffer : *u8 = -56[rbp]
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   mov		eax, DWORD -48[rbp]
   movsx		rax, eax
   push		rax
   pop		rdx
   pop		rcx
   call		realloc
   push		rax
   pop		rax
   mov		QWORD -56[rbp], rax
   mov		rax, -56[rbp]
   push		rax
   push		0
   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L43
   ; block of if
   ; Ln 39 Print
   ; Pop print arguments
   mov		rcx, CS0
   call		printf
   mov		rax, 1
   push		rax
   pop		rcx
   call		exit
   jmp L43
; done
L43:
   ; Ln 43: Assignment
   mov		rax, -56[rbp]
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
   ; Ln 44: Assignment
   mov		eax, DWORD -48[rbp]
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
   lea		rax, [rbx]
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
   mov		eax, DWORD -48[rbp]
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
   call		memset
   push		rax
   pop		rax
   lea		rax, -8[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   lea		rax, [rbx]
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
   mov		rax, -40[rbp]
   push		rax
   mov		eax, DWORD -44[rbp]
   movsx		rax, eax
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   call		memcpy
   push		rax
   pop		rax
   ; Ln 50: Assignment
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
   mov		eax, DWORD -44[rbp]
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
   mov		rax, -40[rbp]
   push		rax
   pop		rcx
   call		free
   jmp L39
; done
L39:
   ; Return
   jmp		L38
L38:
   mov		rax, 0
   add		rsp, 208
   pop		rbp
   ret

; bytes locals   : 16
; bytes temp     : 56
; bytes total    : 112
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
   call		memset
   push		rax
   pop		rax
L44:
   mov		rax, 0
   add		rsp, 112
   pop		rbp
   ret

; bytes locals   : 24
; bytes temp     : 0
; bytes total    : 64
StringBuilder.toString:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64
   ; Param ret_0
   mov		-8[rbp], rcx
   ; Param sb
   mov		-16[rbp], rdx
   ; Return
   lea		rax, -16[rbp]
   push		rax
   pop		rbx
   mov		rbx, [rbx]
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   mov		QWORD [rbx], rax
   jmp		L45
L45:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret

; bytes locals   : 16
; bytes temp     : 24
; bytes total    : 80
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
   call		free
L46:
   mov		rax, 0
   add		rsp, 80
   pop		rbp
   ret

; bytes locals   : 128
; bytes temp     : 24
; bytes total    : 192
Print2:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 192
   ; Param ret_0
   mov		-8[rbp], rcx
   ; Param x
   mov		-12[rbp], edx
   ; Param y
   mov		-16[rbp], r8d
   ; Param z
   mov		-20[rbp], r9d
   ; Param args
   mov		rax, 48[rbp]
   lea		rbx, -40[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 10 Print
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -16[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -20[rbp]
   movsx		rax, eax
   push		rax
   ; Pop print arguments
   pop		rax
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS1
   call		printf
   ; Ln 11: For-loop
   lea		rax, -40[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-64[rbp], rbx     ; data
   mov		-72[rbp], rcx     ; count
   mov		QWORD -80[rbp], 0 ; index
L48:
   mov		rbx, -72[rbp]
   mov		rax, -80[rbp]
   cmp		rax, rbx
   jge		L50
   mov		rbx, QWORD -64[rbp]
   mov		rax, QWORD -80[rbp]
   imul		rax, 16
   lea		rbx, [rbx + rax]
   mov		r8, 16
   mov		rdx, rbx
   lea		rcx, -56[rbp]
   call		memcpy
   ; Ln 12 Print
   lea		rax, -56[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		r9
   lea		rbx, 0[r9]
   mov		eax, DWORD [rbx]
   push		rax
   pop		rdx
   mov		rcx, enum_buffer_0
   call		get_enum_string_TypeKind
   push		rax
   lea		rbx, 8[r9]
   mov		rax, QWORD [rbx]
   push		rax
   lea		rbx, 16[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 20[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   ; Pop print arguments
   pop		rax
   mov		QWORD -136[rbp], rax
   pop		rax
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS2
   mov		rax, -136[rbp]
   mov		QWORD 32[rsp], rax
   call		printf
L49:
   inc		QWORD -80[rbp]
   jmp		L48
L50:
   ; Return
   mov		rax, 5
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   mov		DWORD [rbx], eax
   jmp		L47
L47:
   mov		rax, 0
   add		rsp, 192
   pop		rbp
   ret

; bytes locals   : 176
; bytes temp     : 40
; bytes total    : 256
Print:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 256
   mov		16[rbp], rcx
   mov		24[rbp], rdx
   mov		32[rbp], r8 
   ; Param ret_0
   mov		rax, 16[rbp]
   mov		-8[rbp], rax
   ; Param format
   mov		rax, 24[rbp]
   mov		-16[rbp], rax
   ; Param args
   mov		rax, 32[rbp]
   lea		rbx, -32[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 19: $fmtLen : int = -36[rbp]
   mov		rax, QWORD -16[rbp]
   push		rax
   pop		rcx
   call		strlen
   push		rax
   pop		rax
   mov		DWORD -36[rbp], eax
   ; Ln 20 Print
   mov		rax, QWORD -16[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS3
   call		printf
   ; Ln 21: $sb : StringBuilder = -56[rbp]
   lea		rax, -192[rbp]
   push		rax
   mov		rax, 1024
   push		rax
   pop		rdx
   pop		rcx
   call		NewStringBuilder
   lea		rax, -192[rbp]
   push		rax
   pop		rax
   lea		rbx, -56[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   ; Ln 25: $placeholderIndicies : [..]int = -80[rbp]
   lea		rbx, -80[rbp]
   push		rbx
   mov		rdx, 4
   mov		rcx, 2
   call		calloc
   pop		rbx
   mov		QWORD 0[rbx], rax
   mov		QWORD 8[rbx], 0
   mov		QWORD 16[rbx], 2
   ; Ln 28: Ranged for-loop
   mov		rax, 0
   push		rax
   mov		rax, QWORD -16[rbp]
   push		rax
   pop		rcx
   call		strlen
   push		rax
   pop		rbx
   pop		rax
   mov		QWORD -96[rbp], rax
   mov		QWORD -104[rbp], rbx
   mov		eax, DWORD -96[rbp]
   mov		-88[rbp], eax
L52:
   mov		eax, -104[rbp]
   cmp		-88[rbp], eax
   jge		L54
   ; Ln 29: $c : u8 = -105[rbp]
   mov		rax, QWORD -16[rbp]
   push		rax
   mov		eax, DWORD -88[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   pop		rbx
   imul		rax, 1; Add index expr
   add		rbx, rax
   movzx		eax, BYTE [rbx]
   push		rax
   pop		rax
   mov		BYTE -105[rbp], al
   ; Ln 30 Print
   movzx		eax, BYTE -105[rbp]
   push		rax
   ; Pop print arguments
   pop		rax
   movzx		rax, al
   mov		rdx, rax
   mov		rcx, CS4
   call		printf
L53:
   inc		DWORD -88[rbp]
   jmp		L52
L54:
   ; Ln 33: For-loop
   lea		rax, -32[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-129[rbp], rbx     ; data
   mov		-137[rbp], rcx     ; count
   mov		QWORD -145[rbp], 0 ; index
L55:
   mov		rbx, -137[rbp]
   mov		rax, -145[rbp]
   cmp		rax, rbx
   jge		L57
   mov		rbx, QWORD -129[rbp]
   mov		rax, QWORD -145[rbp]
   imul		rax, 16
   lea		rbx, [rbx + rax]
   mov		r8, 16
   mov		rdx, rbx
   lea		rcx, -121[rbp]
   call		memcpy
   ; Ln 34 Print
   lea		rax, -121[rbp]
   push		rax
   pop		rbx
   add		rbx, 8
   push		rbx
   pop		rbx
   mov		rax, [rbx]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		r9
   lea		rbx, 0[r9]
   mov		eax, DWORD [rbx]
   push		rax
   pop		rdx
   mov		rcx, enum_buffer_0
   call		get_enum_string_TypeKind
   push		rax
   lea		rbx, 8[r9]
   mov		rax, QWORD [rbx]
   push		rax
   lea		rbx, 16[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   lea		rbx, 20[r9]
   mov		eax, DWORD [rbx]
   movsx		rax, eax
   push		rax
   ; Pop print arguments
   pop		rax
   mov		QWORD -184[rbp], rax
   pop		rax
   mov		r9, rax
   pop		rax
   mov		r8, rax
   pop		rax
   mov		rdx, rax
   mov		rcx, CS5
   mov		rax, -184[rbp]
   mov		QWORD 32[rsp], rax
   call		printf
L56:
   inc		QWORD -145[rbp]
   jmp		L55
L57:
   lea		rax, -56[rbp]
   push		rax
   pop		rcx
   call		StringBuilder.free
   ; Return
   mov		rax, 0
   push		rax
   pop		rax
   mov		rbx, -8[rbp]
   mov		DWORD [rbx], eax
   jmp		L51
L51:
   mov		rax, 0
   add		rsp, 256
   pop		rbp
   ret
get_enum_string_HttpStatus:
   mov		r8, 200
   cmp		rdx, r8
   jz			enum_case_58
   mov		r8, 201
   cmp		rdx, r8
   jz			enum_case_59
   mov		r8, 400
   cmp		rdx, r8
   jz			enum_case_60
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
enum_case_58:
   mov		rax, __HttpStatus.Ok
   ret
enum_case_59:
   mov		rax, __HttpStatus.Created
   ret
enum_case_60:
   mov		rax, __HttpStatus.BadRequest
   ret

; bytes locals   : 184
; bytes temp     : 248
; bytes total    : 464
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 464
   ; Ln 14: $written : int = -4[rbp]
   lea		rax, -188[rbp]
   push		rax
   mov		rax, CS6
   push		rax
   mov		rax, 1
   push		rax
   pop		rax
   mov		DWORD -108[rbp], eax
   lea		rax, -108[rbp]
   lea		rbx, -204[rbp]
   mov		QWORD 0[rbx], rax
   mov		rax, QWORD [Type_int]
   push		rax
   pop		rcx
   lea		rbx, -204[rbp]
   mov		QWORD 8[rbx], rcx
   push		rbx
   pop		rax
   lea		rbx, -104[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   movss		xmm0, [CF7]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-112[rbp], eax
   lea		rax, -112[rbp]
   lea		rbx, -220[rbp]
   mov		QWORD 0[rbx], rax
   mov		rax, QWORD [Type_float]
   push		rax
   pop		rcx
   lea		rbx, -220[rbp]
   mov		QWORD 8[rbx], rcx
   push		rbx
   pop		rax
   lea		rbx, -88[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   push		1
   pop		rax
   mov		BYTE -113[rbp], al
   lea		rax, -113[rbp]
   lea		rbx, -236[rbp]
   mov		QWORD 0[rbx], rax
   mov		rax, QWORD [Type_bool]
   push		rax
   pop		rcx
   lea		rbx, -236[rbp]
   mov		QWORD 8[rbx], rcx
   push		rbx
   pop		rax
   lea		rbx, -72[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   mov		rax, CS8
   push		rax
   pop		rax
   mov		QWORD -128[rbp], rax
   lea		rax, -128[rbp]
   lea		rbx, -252[rbp]
   mov		QWORD 0[rbx], rax
   mov		rax, QWORD [Type_string]
   push		rax
   pop		rcx
   lea		rbx, -252[rbp]
   mov		QWORD 8[rbx], rcx
   push		rbx
   pop		rax
   lea		rbx, -56[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   push		200
   pop		rax
   mov		DWORD -132[rbp], eax
   lea		rax, -132[rbp]
   lea		rbx, -268[rbp]
   mov		QWORD 0[rbx], rax
   mov		rax, QWORD [Type_int]
   push		rax
   mov		rcx, CS9
   pop		rdx
   mov		r8d, 0
   mov		r9d, 400
   sub		rsp, 32
   call		runtime_get_type_enum
   add		rsp, 32
   push		rax
   pop		rcx
   lea		rbx, -268[rbp]
   mov		QWORD 8[rbx], rcx
   push		rbx
   pop		rax
   lea		rbx, -40[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rcx, -280[rbp]
   mov		rdx, 0
   mov		r8, 12
   call		memset
   movss		xmm0, [CF10]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-280[rbp], eax
   movss		xmm0, [CF11]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-276[rbp], eax
   movss		xmm0, [CF12]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-272[rbp], eax
   lea		rax, -280[rbp]
   push		rax
   pop		rax
   lea		rbx, -144[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 12
   call		memcpy
   lea		rax, -144[rbp]
   lea		rbx, -296[rbp]
   mov		QWORD 0[rbx], rax
   sub		rsp, 32
   mov		rcx, 72
   call		malloc
   add		rsp, 32
   mov		-304[rbp], rax
   mov		-312[rbp], rax
   mov		rax, QWORD [Type_float]
   push		rax
   pop		rcx
   mov		rbx, -312[rbp]
   mov		rax, CS13
   mov		0[rbx], rax
   mov		8[rbx], rcx
   mov		DWORD 16[rbx], 0
   mov		DWORD 20[rbx], 0
   add		rbx, 24
   mov		-312[rbp], rbx
   mov		rax, QWORD [Type_float]
   push		rax
   pop		rcx
   mov		rbx, -312[rbp]
   mov		rax, CS14
   mov		0[rbx], rax
   mov		8[rbx], rcx
   mov		DWORD 16[rbx], 1
   mov		DWORD 20[rbx], 4
   add		rbx, 24
   mov		-312[rbp], rbx
   mov		rax, QWORD [Type_float]
   push		rax
   pop		rcx
   mov		rbx, -312[rbp]
   mov		rax, CS15
   mov		0[rbx], rax
   mov		8[rbx], rcx
   mov		DWORD 16[rbx], 2
   mov		DWORD 20[rbx], 8
   add		rbx, 24
   mov		-312[rbp], rbx
   mov		rcx, CS16
   mov		rdx, -304[rbp]
   mov		r8d, 3
   mov		r9d, 12
   sub		rsp, 40
   mov		DWORD 32[rsp], 4
   call		runtime_get_type_struct
   add		rsp, 40
   push		rax
   pop		rcx
   lea		rbx, -296[rbp]
   mov		QWORD 8[rbx], rcx
   push		rbx
   pop		rax
   lea		rbx, -24[rbp]
   lea		rcx, 0[rbx]
   lea		rdx, 0[rax]
   mov		r8, 16
   call		memcpy
   lea		rax, -104[rbp]
   mov		QWORD -328[rbp], rax
   mov		QWORD -320[rbp], 6
   lea		rax, -328[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		QWORD -344[rbp], rbx
   mov		QWORD -336[rbp], rcx
   lea		rax, -344[rbp]
   push		rax
   pop		r8 
   pop		rdx
   pop		rcx
   call		Print
   mov		eax, DWORD -188[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -4[rbp], eax
L62:
   mov		rax, 0
   add		rsp, 464
   pop		rbp
   ret
