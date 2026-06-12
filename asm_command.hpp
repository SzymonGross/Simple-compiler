#pragma once

#include "varible.hpp"
#include "tree.hpp"

#include <string>
#include <unordered_map>
#include <vector>

enum class Op
{
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
    Jne,
    Jmp,
    Lea,
    And,
    Or,
    Sal,
    Sar,
    Cdq,
    Cqo,
    Idiv,
    Sete,
    Setne,
    Setl,
    Setle,
    Setg,
    Setge,
    Movzx,
    Call,
    Test,
    Xor,
    Je,
    Jl,
    Jg,
    Jge,
    Asciz
};

struct AsmCommand
{
    Op op{};
    std::string a;
    std::string b;

    AsmCommand() = default;
    explicit AsmCommand(Op operation, std::string argA = {}, std::string argB = {})
        : op(operation), a(std::move(argA)), b(std::move(argB)) {}

    std::string emit() const;
};

class Generator
{
    std::vector<bool> stack = {1};
    std::unordered_map<std::string, Varbile *> mem;

public:
    bool contains_printf = false;
    bool contains_scanf = false;

    std::vector<AsmCommand> data;
    std::vector<AsmCommand> body;

private:
    std::pair<std::vector<AsmCommand>, std::vector<AsmCommand>> gen_command(Tree::Node *node);
    void generator_step(Tree::Node *node);
    void mem_aloc(Tree::Node *node);
    bool worth_storing(const std::string &s, const std::size_t beg) const;

public:
    bool Piphole_opt();
    void compact_stack_offsets();
    Generator(Tree tree);
};
