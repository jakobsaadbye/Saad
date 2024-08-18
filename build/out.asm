bits 64
default rel

segment .data
   fmt_int   DB "%d", 10, 0
   fmt_float DB "%f", 10, 0
   fmt_string DB "%s", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0
   string_assert_fail  DB "Assertion failed at line %d", 10, 0
   __CardValue.three DB "CardValue.three", 10, 0
   __CardValue.ace DB "CardValue.ace", 10, 0
   __CardValue.seven DB "CardValue.seven", 10, 0
   __CardValue.eight DB "CardValue.eight", 10, 0
   __CardValue.jack DB "CardValue.jack", 10, 0
   __CardValue.king DB "CardValue.king", 10, 0
   __CardValue.count DB "CardValue.count", 10, 0
   __CardValue.two DB "CardValue.two", 10, 0
   __CardValue.five DB "CardValue.five", 10, 0
   __CardValue.nine DB "CardValue.nine", 10, 0
   __CardValue.joker DB "CardValue.joker", 10, 0
   __CardValue.four DB "CardValue.four", 10, 0
   __CardValue.six DB "CardValue.six", 10, 0
   __CardValue.queen DB "CardValue.queen", 10, 0
   __CardValue.ten DB "CardValue.ten", 10, 0
   __CardSuit.spades DB "CardSuit.spades", 10, 0
   __CardSuit.clubs DB "CardSuit.clubs", 10, 0
   __CardSuit.count DB "CardSuit.count", 10, 0
   __CardSuit.joker DB "CardSuit.joker", 10, 0
   __CardSuit.diamonds DB "CardSuit.diamonds", 10, 0
   __CardSuit.hearts DB "CardSuit.hearts", 10, 0

segment .text
   global main
   extern printf
   extern ExitProcess

assert:
   cmp		cl, 0
   jz 		assert_fail
   ret
assert_fail:
   mov		rcx, string_assert_fail
   call		printf
   mov		rcx, 1
   call		ExitProcess

enum_str:
   mov		rcx, fmt_string
   jmp		enum_end
enum_int:
   mov		rcx, fmt_int
   mov		rdx, rax
enum_end:
   pop		rbx
   call		printf
   push		rbx
   ret

print_enum_CardValue:
   mov		rbx, 3
   cmp		rax, rbx
   jz			enum_case_0
   mov		rbx, 1
   cmp		rax, rbx
   jz			enum_case_2
   mov		rbx, 7
   cmp		rax, rbx
   jz			enum_case_3
   mov		rbx, 8
   cmp		rax, rbx
   jz			enum_case_4
   mov		rbx, 11
   cmp		rax, rbx
   jz			enum_case_5
   mov		rbx, 13
   cmp		rax, rbx
   jz			enum_case_6
   mov		rbx, 14
   cmp		rax, rbx
   jz			enum_case_7
   mov		rbx, 2
   cmp		rax, rbx
   jz			enum_case_8
   mov		rbx, 5
   cmp		rax, rbx
   jz			enum_case_9
   mov		rbx, 9
   cmp		rax, rbx
   jz			enum_case_10
   mov		rbx, 0
   cmp		rax, rbx
   jz			enum_case_11
   mov		rbx, 4
   cmp		rax, rbx
   jz			enum_case_12
   mov		rbx, 6
   cmp		rax, rbx
   jz			enum_case_13
   mov		rbx, 12
   cmp		rax, rbx
   jz			enum_case_14
   mov		rbx, 10
   cmp		rax, rbx
   jz			enum_case_15
   jmp		enum_int
enum_case_0:
   mov		rdx, __CardValue.three
   jmp		enum_str
enum_case_2:
   mov		rdx, __CardValue.ace
   jmp		enum_str
enum_case_3:
   mov		rdx, __CardValue.seven
   jmp		enum_str
enum_case_4:
   mov		rdx, __CardValue.eight
   jmp		enum_str
enum_case_5:
   mov		rdx, __CardValue.jack
   jmp		enum_str
enum_case_6:
   mov		rdx, __CardValue.king
   jmp		enum_str
enum_case_7:
   mov		rdx, __CardValue.count
   jmp		enum_str
enum_case_8:
   mov		rdx, __CardValue.two
   jmp		enum_str
enum_case_9:
   mov		rdx, __CardValue.five
   jmp		enum_str
enum_case_10:
   mov		rdx, __CardValue.nine
   jmp		enum_str
enum_case_11:
   mov		rdx, __CardValue.joker
   jmp		enum_str
enum_case_12:
   mov		rdx, __CardValue.four
   jmp		enum_str
enum_case_13:
   mov		rdx, __CardValue.six
   jmp		enum_str
enum_case_14:
   mov		rdx, __CardValue.queen
   jmp		enum_str
enum_case_15:
   mov		rdx, __CardValue.ten
   jmp		enum_str
print_enum_CardSuit:
   mov		rbx, 4
   cmp		rax, rbx
   jz			enum_case_17
   mov		rbx, 2
   cmp		rax, rbx
   jz			enum_case_19
   mov		rbx, 5
   cmp		rax, rbx
   jz			enum_case_20
   mov		rbx, 0
   cmp		rax, rbx
   jz			enum_case_21
   mov		rbx, 1
   cmp		rax, rbx
   jz			enum_case_22
   mov		rbx, 3
   cmp		rax, rbx
   jz			enum_case_23
   jmp		enum_int
enum_case_17:
   mov		rdx, __CardSuit.spades
   jmp		enum_str
enum_case_19:
   mov		rdx, __CardSuit.clubs
   jmp		enum_str
enum_case_20:
   mov		rdx, __CardSuit.count
   jmp		enum_str
enum_case_21:
   mov		rdx, __CardSuit.joker
   jmp		enum_str
enum_case_22:
   mov		rdx, __CardSuit.diamonds
   jmp		enum_str
enum_case_23:
   mov		rdx, __CardSuit.hearts
   jmp		enum_str

main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64
   push		0
   push		5
   pop		rbx
   pop		rax
   mov		-4[rbp], eax
   mov		-8[rbp], ebx
   mov		eax, -4[rbp]
   mov		-12[rbp], eax
L26:
   mov		eax, -8[rbp]
   cmp		-12[rbp], eax
   jge		L27
   push		0
   push		14
   pop		rbx
   pop		rax
   mov		-16[rbp], eax
   mov		-20[rbp], ebx
   mov		eax, -16[rbp]
   mov		-24[rbp], eax
L28:
   mov		eax, -20[rbp]
   cmp		-24[rbp], eax
   jge		L29

   ; initialization of 'c'
   mov		DWORD -32[rbp], 0
   mov		DWORD -28[rbp], 0
   mov		eax, DWORD -12[rbp]
   push		rax
   pop		rax
   mov		DWORD -28[rbp], eax
   mov		eax, DWORD -24[rbp]
   push		rax
   pop		rax
   mov		DWORD -32[rbp], eax

   ; expression of print
   mov		eax, DWORD -28[rbp]
   push		rax

   ; call to print
   pop		rax
   call		print_enum_CardSuit

   ; expression of print
   mov		eax, DWORD -32[rbp]
   push		rax

   ; call to print
   pop		rax
   call		print_enum_CardValue
   inc		DWORD -24[rbp]
   jmp		L28
L29:
   inc		DWORD -12[rbp]
   jmp		L26
L27:
L25:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret
