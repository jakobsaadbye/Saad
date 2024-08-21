bits 64
default rel

segment .data
   fmt_int   DB "%lld", 10, 0
   fmt_float DB "%lf", 10, 0
   fmt_string DB "%s", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0
   string_assert_fail  DB "Assertion failed at line %d", 10, 0

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


main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 80

   ; initialization of 'a'
   mov		BYTE -1[rbp], 0
   mov		rax, 255
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax


   ; putting result into 'a'
   pop		rax
   mov		BYTE -1[rbp], al

   ; initialization of 'b'
   mov		WORD -4[rbp], 0
   mov		rax, 65535
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax


   ; putting result into 'b'
   pop		rax
   mov		WORD -4[rbp], ax

   ; initialization of 'c'
   mov		DWORD -8[rbp], 0
   mov		rax, 4294967295
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax


   ; putting result into 'c'
   pop		rax
   mov		DWORD -8[rbp], eax

   ; initialization of 'd'
   mov		QWORD -16[rbp], 0
   mov		rax, 18446744073709551615
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax


   ; putting result into 'd'
   pop		rax
   mov		QWORD -16[rbp], rax
   mov		rax, 0
   mov		al, BYTE -1[rbp]
   push		rax
   mov		rax, 0
   push		rax

   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 7
   call		assert
   mov		rax, 0
   mov		ax, WORD -4[rbp]
   push		rax
   mov		rax, 0
   push		rax

   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 8
   call		assert
   mov		rax, 0
   mov		eax, DWORD -8[rbp]
   push		rax
   mov		rax, 0
   push		rax

   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 9
   call		assert
   mov		rax, 0
   mov		rax, QWORD -16[rbp]
   push		rax
   mov		rax, 0
   push		rax

   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 10
   call		assert

   ; initialization of 'e'
   mov		BYTE -17[rbp], 0
   mov		rax, 127
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax


   ; putting result into 'e'
   pop		rax
   mov		BYTE -17[rbp], al

   ; initialization of 'f'
   mov		WORD -20[rbp], 0
   mov		rax, 32767
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax


   ; putting result into 'f'
   pop		rax
   mov		WORD -20[rbp], ax

   ; initialization of 'g'
   mov		DWORD -24[rbp], 0
   mov		rax, 2147483647
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax


   ; putting result into 'g'
   pop		rax
   mov		DWORD -24[rbp], eax

   ; initialization of 'h'
   mov		QWORD -32[rbp], 0
   mov		rax, 9223372036854775807
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax


   ; putting result into 'h'
   pop		rax
   mov		QWORD -32[rbp], rax

   ; initialization of 'i'
   mov		BYTE -33[rbp], 0
   mov		rax, 129
   push		rax
   pop		rax
   neg		rax
   push		rax


   ; putting result into 'i'
   pop		rax
   mov		BYTE -33[rbp], al

   ; initialization of 'j'
   mov		WORD -36[rbp], 0
   mov		rax, 32769
   push		rax
   pop		rax
   neg		rax
   push		rax


   ; putting result into 'j'
   pop		rax
   mov		WORD -36[rbp], ax

   ; initialization of 'k'
   mov		DWORD -40[rbp], 0
   mov		rax, 2147483649
   push		rax
   pop		rax
   neg		rax
   push		rax


   ; putting result into 'k'
   pop		rax
   mov		DWORD -40[rbp], eax

   ; initialization of 'l'
   mov		QWORD -48[rbp], 0
   mov		rax, 9223372036854775809
   push		rax
   pop		rax
   neg		rax
   push		rax


   ; putting result into 'l'
   pop		rax
   mov		QWORD -48[rbp], rax
   mov		rax, 0
   mov		al, BYTE -17[rbp]
   movsx		rax, al
   push		rax
   mov		rax, 128
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
   mov		rdx, 22
   call		assert
   mov		rax, 0
   mov		ax, WORD -20[rbp]
   movsx		rax, ax
   push		rax
   mov		rax, 32768
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
   mov		rdx, 23
   call		assert
   mov		rax, 0
   mov		eax, DWORD -24[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 2147483648
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
   mov		rdx, 24
   call		assert
   mov		rax, 0
   mov		rax, QWORD -32[rbp]
   push		rax
   mov		rax, 9223372036854775808
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
   mov		rdx, 25
   call		assert
   mov		rax, 0
   mov		al, BYTE -33[rbp]
   movsx		rax, al
   push		rax
   mov		rax, 127
   push		rax

   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 26
   call		assert
   mov		rax, 0
   mov		ax, WORD -36[rbp]
   movsx		rax, ax
   push		rax
   mov		rax, 32767
   push		rax

   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 27
   call		assert
   mov		rax, 0
   mov		eax, DWORD -40[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 2147483647
   push		rax

   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 28
   call		assert
   mov		rax, 0
   mov		rax, QWORD -48[rbp]
   push		rax
   mov		rax, 9223372036854775807
   push		rax

   pop		rbx
   pop		rax
   cmp		rax, rbx
   sete		al
   push		rax
   pop		rcx
   mov		rdx, 29
   call		assert
L0:
   mov		rax, 0
   add		rsp, 80
   pop		rbp
   ret
