.intel_syntax noprefix
.global main
.extern printf
.extern scanf

.section .data
main_text_1:
    .asciz "HELLO WORLD!\n"
text_main2:
    .asciz "%lld%d"
main_text_2:
    .asciz "So x is %lld\n"
text_main3:
    .asciz "So x+y is %lld\n"

.section .text
main:
    push rbp
    mov rbp, rsp
    sub rsp, 48
    lea rdi, [rip + main_text_1]
    xor eax, eax
    call printf

    mov qword ptr [rbp-8], 0
    lea rdi, [rip + text_main2]
    lea rsi, [rbp-8]
    lea rdx, [rbp-12]
    xor eax, eax
    call scanf

    lea rdi, [rip + main_text_2]
    mov rsi, qword ptr [rbp-8]
    xor eax, eax
    call printf

    mov rax, qword ptr [rbp-8]
    mov ecx, dword ptr [rbp-12]
    add rax, rcx
    mov qword ptr [rbp-8], rax
    lea rdi, [rip + text_main3]
    mov rsi, qword ptr [rbp-8]
    xor eax, eax
    call printf

    mov eax, 0
    add rsp, 48
    pop rbp
    ret

