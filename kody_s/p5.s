.intel_syntax noprefix
.global main
.section .text
fun:
    push rbp
    mov rbp, rsp
    sub rsp, 48
    mov dword ptr [rbp-4], ecx
    mov dword ptr [rbp-8], edx
    test ecx, ecx
    jne endif_0

    mov eax, edx
    add eax, 1
    jmp blockend_0

endif_0:
    cmp dword ptr [rbp-8], 0
    jne endif_1

    mov eax, dword ptr [rbp-4]
    sub eax, 1
    mov ecx, eax
    mov edx, 1
    call fun

    jmp blockend_0

endif_1:
    mov eax, dword ptr [rbp-4]
    sub eax, 1
    mov dword ptr [rbp-29], eax
    mov ecx, dword ptr [rbp-4]
    mov eax, dword ptr [rbp-8]
    sub eax, 1
    mov edx, eax
    call fun

    mov ecx, dword ptr [rbp-29]
    mov edx, eax
    call fun

blockend_0:
    add rsp, 48
    pop rbp
    ret

main:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    mov ecx, 3
    mov edx, 3
    call fun

    add rsp, 16
    pop rbp
    ret

