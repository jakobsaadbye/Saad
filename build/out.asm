bits 64
default rel

segment .data
   fmt_int   DB "%d", 10, 0
   fmt_float DB "%f", 10, 0
   fmt_string DB "%s", 10, 0
   string_false DB "false", 10, 0
   string_true  DB "true", 10, 0
   CF0 DD 3.141593

segment .text
   global main
   extern ExitProcess
   extern printf

main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 48

   ; initialization of 'pi'
   mov		DWORD [rbp - 4], 0
   movss		xmm0, [CF0]
   sub		rsp, 4
   movss		[rsp], xmm0


   ; putting result into 'pi'
   movss		xmm0, [rsp]
   add		rsp, 4
   movss		DWORD [rbp - 4], xmm0

   ; initialization of 'tau'
   mov		DWORD [rbp - 8], 0
   movss		xmm0, [rbp - 4]
   sub		rsp, 4
   movss		[rsp], xmm0

   ; casting value 2 to a float
   mov		rax, 2
   cvtsi2ss	xmm0, rax
   sub		rsp, 4
   movss		[rsp], xmm0

   movss		xmm1, [rsp]
   add		rsp, 4
   movss		xmm0, [rsp]
   add		rsp, 4
   mulss		xmm0, xmm1
   sub		rsp, 4
   movss		[rsp], xmm0


   ; putting result into 'tau'
   movss		xmm0, [rsp]
   add		rsp, 4
   movss		DWORD [rbp - 8], xmm0

   ; initialization of 'pi_again'
   mov		DWORD [rbp - 12], 0
   movss		xmm0, [rbp - 8]
   sub		rsp, 4
   movss		[rsp], xmm0

   ; casting value 2 to a float
   mov		rax, 2
   cvtsi2ss	xmm0, rax
   sub		rsp, 4
   movss		[rsp], xmm0
   movss		xmm1, [rsp]
   add		rsp, 4
   movss		xmm0, [rsp]
   add		rsp, 4
   divss		xmm0, xmm1
   sub		rsp, 4
   movss		[rsp], xmm0


   ; putting result into 'pi_again'
   movss		xmm0, [rsp]
   add		rsp, 4
   movss		DWORD [rbp - 12], xmm0

   ; expression of print
   movss		xmm0, [rbp - 12]
   sub		rsp, 4
   movss		[rsp], xmm0

   ; call to print
   movss		xmm0, [rsp]
   add		rsp, 4
   cvtss2sd	xmm0, xmm0
   movapd	xmm1, xmm0
   movq		rdx, xmm0
   mov		rcx, fmt_float
   call		printf

   ; expression of print
   movss		xmm0, [rbp - 8]
   sub		rsp, 4
   movss		[rsp], xmm0

   ; call to print
   movss		xmm0, [rsp]
   add		rsp, 4
   cvtss2sd	xmm0, xmm0
   movapd	xmm1, xmm0
   movq		rdx, xmm0
   mov		rcx, fmt_float
   call		printf

   ; cleanup the stack
   pop		rbp
   add		rsp, 48
   mov		rcx, 0
   call		ExitProcess
