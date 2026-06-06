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
    Jge
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
    std::size_t ofs = 0;
    std::size_t max_ofs = 32;
    std::unordered_map<std::string, Varbile *> mem;

public:
    std::vector<AsmCommand> body;

private:
    std::pair<std::vector<AsmCommand>, std::vector<AsmCommand>> gen_command(Tree::Node *node);
    void generator_step(Tree::Node *node);
    void mem_aloc(Tree::Node *node);
    bool worth_storing(const std::string &s, const std::size_t beg) const;

public:
    bool Piphole_opt();
    Generator(Tree tree);
};
