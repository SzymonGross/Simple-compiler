.intel_syntax noprefix
.global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 60
    mov eax, 13
    add rsp, 60
    pop rbp
    ret
