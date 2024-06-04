bits 64
default rel

segment .data
   fmt_int   DB "%d", 10, 0
   fmt_float DB "%f", 10, 0
   fmt_string DB "%s", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0

segment .text
   global main
   extern printf
   extern ExitProcess


; for i in 0..20 { print(i); }

main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64

   ; save range values
   mov      DWORD -4[rbp], 5
   mov      DWORD -8[rbp], 10

   ; initialize i
   mov      eax, DWORD -4[rbp]
   mov      DWORD -12[rbp], eax

FOR:
   ; test
   mov      eax, DWORD -8[rbp]
   cmp      DWORD -12[rbp], eax
   jg       DONE

   ; print
   mov      rcx, fmt_int
   mov      edx, DWORD -12[rbp]
   call     printf

   ; increment i
   inc      DWORD -12[rbp]

   ; loop back
   jmp      FOR

DONE:
   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret
   
