.intel_syntax noprefix
.global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    mov dword ptr [rbp-4], 15
    mov dword ptr [rbp-8], 12
loop_0:
    mov eax, dword ptr [rbp-4]
    mov ecx, dword ptr [rbp-8]
    cmp eax, ecx
    je endif_0

    jge endif_1

    mov dword ptr [rbp-4], ecx
    mov dword ptr [rbp-8], eax
endif_1:
    mov eax, dword ptr [rbp-4]
    sub eax, dword ptr [rbp-8]
    mov dword ptr [rbp-4], eax
    jmp loop_0

endif_0:
    mov eax, dword ptr [rbp-4]
    add rsp, 32
    pop rbp
    ret
