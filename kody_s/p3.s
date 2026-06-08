.intel_syntax noprefix
.global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    mov eax, 8
    add rsp, 32
    pop rbp
    ret
