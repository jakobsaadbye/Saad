bits 64
default rel

segment .data
   fmt_int   DB "%d", 10, 0
   fmt_float DB "%f", 10, 0
   fmt_string DB "%s", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0
   string_assert_fail  DB "Assertion failed at line %d", 10, 0

segment .text
   global main
   extern printf
   extern ExitProcess


; assert(false);

assert:
   cmp      cl, 0
   jz       assert_fail
   ret

assert_fail:
   lea      rcx, [string_assert_fail] 
   call     printf

   mov      rcx, 1
   call     ExitProcess

main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 64

   mov      rcx, 0
   mov      rdx, 7
   call     assert

   mov		rax, 0
   add		rsp, 64
   pop		rbp
   ret
   
