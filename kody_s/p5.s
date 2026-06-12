.intel_syntax noprefix
.global main
.extern printf
.extern scanf

.section .data
main_text_1:
    .asciz "%d%d"
text_main2:
    .asciz "%d\n"

.section .text
fun:
    push rbp
    mov rbp, rsp
    sub rsp, 48
    mov dword ptr [rbp-4], ecx
    mov dword ptr [rbp-8], edx
    test ecx, ecx
    jne fun_endif_0

    mov eax, edx
    add eax, 1
    jmp fun_blockend_0

fun_endif_0:
    cmp dword ptr [rbp-8], 0
    jne fun_endif_1

    mov ecx, dword ptr [rbp-4]
    sub ecx, 1
    mov edx, 1
    call fun

    jmp fun_blockend_0

fun_endif_1:
    mov eax, dword ptr [rbp-4]
    sub eax, 1
    mov dword ptr [rbp-12], eax
    mov ecx, dword ptr [rbp-4]
    mov edx, dword ptr [rbp-8]
    sub edx, 1
    call fun

    mov ecx, dword ptr [rbp-12]
    mov edx, eax
    call fun

fun_blockend_0:
    add rsp, 48
    pop rbp
    ret

main:
    push rbp
    mov rbp, rsp
    sub rsp, 48
    lea rcx, [rip + main_text_1]
    lea rdx, [rbp-4]
    lea r8, [rbp-8]
    xor eax, eax
    call scanf

    mov ecx, dword ptr [rbp-4]
    mov edx, dword ptr [rbp-8]
    call fun

    lea rcx, [rip + text_main2]
    mov edx, eax
    xor eax, eax
    call printf

    mov eax, 0
    add rsp, 48
    pop rbp
    ret

