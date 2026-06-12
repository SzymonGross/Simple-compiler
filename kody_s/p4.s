.intel_syntax noprefix
.global main

.section .text
main:
    push rbp
    mov rbp, rsp
    sub rsp, 48
    mov dword ptr [rbp-4], 98765
    mov dword ptr [rbp-8], 0
loop_0:
    cmp dword ptr [rbp-4], 0
    jle endif_0

    mov eax, dword ptr [rbp-4]
    cdq
    mov ecx, 10
    idiv ecx
    mov eax, dword ptr [rbp-8]
    add eax, edx
    mov dword ptr [rbp-8], eax
    mov eax, dword ptr [rbp-4]
    cdq
    mov ecx, 10
    idiv ecx
    mov dword ptr [rbp-4], eax
    jmp loop_0

endif_0:
    cmp dword ptr [rbp-8], 10
    jge endif_1

    mov dword ptr [rbp-4], 1
    jmp blockend_0

endif_1:
    cmp dword ptr [rbp-8], 25
    jge endif_2

    mov dword ptr [rbp-4], 2
    jmp blockend_0

endif_2:
    cmp dword ptr [rbp-8], 40
    jge endif_3

    mov dword ptr [rbp-4], 3
    jmp blockend_0

endif_3:
    mov dword ptr [rbp-4], 4
blockend_0:
    mov eax, dword ptr [rbp-4]
    add rsp, 48
    pop rbp
    ret

