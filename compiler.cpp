#include "compiler.hpp"
#include "token.hpp"
#include "tree.hpp"
#include "asm_command.hpp"

#include <sstream>
#include <vector>
#include <string>

Compiler::Compiler(std::istream &input)
{
    Tree tree{};

    std::string line;
    while (std::getline(input, line))
        tree.append(Token(line));

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
    Generator gen(tree);

    changed = 1;
    while (changed)
    {
        changed = 0;
        changed |= gen.Piphole_opt();
    }
    code = move(gen.body);
}

void Compiler::write(std::ostream &output) const
{
    for (const AsmCommand &a : code)
    {
        if (a.op != Op::Directive && a.op != Op::Label)
            output << "    " << a.emit() << "\n";
        else
            output << a.emit() << "\n";
    }
}
