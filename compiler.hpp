#pragma once

#include "asm_command.hpp"

#include <sstream>
#include <vector>

class Compiler
{
    std::vector<AsmCommand> code;
public:
    explicit Compiler(std::istream &input);
    void write(std::ostream &output) const;
};
