#include "compiler.hpp"
#include "token.hpp"
#include "tree.hpp"
#include "asm_command.hpp"
#include "mask.hpp"

#include <sstream>
#include <vector>
#include <string>
#include <iostream>

Compiler::Compiler(std::istream &input)
{
    import.emplace_back(Op::Directive, ".intel_syntax noprefix");
    import.emplace_back(Op::Directive, ".global main");

    code.emplace_back(Op::Directive, ".section .text");

    std::vector<Tree> forest;

    std::istringstream temp = mask(input);

    bool contains_printf = false;
    bool contains_scanf = false;

    std::string line;
    while (std::getline(temp, line))
    {
        std::cout << line << "\n";
        Token tok(line);

        if (tok.name == "funkcja")
        {
            fun_type[tok.arg[1]] = tok.arg[0];

            for (int i = 2; i < tok.arg.size(); i++)
                if (i % 2 == 0)
                    fun_args[tok.arg[1]].push_back(tok.arg[i]);

            forest.emplace_back();
            forest.back().get_beg()->arg = tok.arg;
        }
        else
            forest.back().append(tok);
    }

    for (Tree tree : forest)
    {
        tree.fun_type = fun_type;
        tree.fun_args = fun_args;

        tree.print();
        bool changed = 1;
        while (changed)
        {
            changed = 0;
            changed |= tree.loop_unrolling();
            tree.traversal();

            changed |= tree.array_access_lowering();
            changed |= tree.constant_folding();
            changed |= tree.identity_simplification();
            changed |= tree.branch_cutting();
            changed |= tree.assignment_combining();
            changed |= tree.varible_cleaner();
            changed |= tree.neg_simplification();
            changed |= tree.identity_simplification();
        }

        tree.release_dead_variables();

        tree.print();

        Generator gen(tree);

        changed = 1;
        while (changed)
        {
            changed = 0;
            changed |= gen.Piphole_opt();
        }
        gen.compact_stack_offsets();

        contains_printf |= gen.contains_printf;
        contains_scanf |= gen.contains_scanf;

        for (const auto &a : gen.data)
            data.push_back(a);

        for (const auto &a : gen.body)
            code.push_back(a);
    }

    if (contains_printf)
        import.emplace_back(Op::Directive, ".extern printf");
    if (contains_scanf)
        import.emplace_back(Op::Directive, ".extern scanf");
}

void Compiler::write(std::ostream &output) const
{
    for (const AsmCommand &a : import)
    {
        if (a.op != Op::Directive && a.op != Op::Label)
            output << "    " << a.emit() << "\n";
        else
            output << a.emit() << "\n";
    }

    output << "\n";
    if (!data.empty())
        output << ".section .data\n";

    for (const AsmCommand &a : data)
    {
        if (a.op != Op::Directive && a.op != Op::Label)
            output << "    " << a.emit() << "\n";
        else
            output << a.emit() << "\n";
    }

    if (!data.empty())
        output << "\n";

    for (const AsmCommand &a : code)
    {
        if (a.op != Op::Directive && a.op != Op::Label)
            output << "    " << a.emit() << "\n";
        else
            output << a.emit() << "\n";
    }
}
