#pragma once

#include <string>

enum class Op {
    Mov,
    Add,
    Sub,
    Imul,
    Push,
    Pop,
    Ret,
    Label,
    Directive,
    Cmp,
    Jle,
    Jmp,
};

struct AsmCommand {
    Op op{};
    std::string a;
    std::string b;

    AsmCommand() = default;
    explicit AsmCommand(Op operation, std::string argA = {}, std::string argB = {})
        : op(operation), a(std::move(argA)), b(std::move(argB)) {}

    std::string emit() const;
};
