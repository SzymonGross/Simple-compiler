#pragma once

#include "asm_command.hpp"

#include <sstream>
#include <vector>

class Compiler
{
    std::unordered_map<std::string, std::string> fun_type;
    std::unordered_map<std::string, std::vector<std::string>> fun_args;

    std::vector<AsmCommand> import;
    std::vector<AsmCommand> data;
    std::vector<AsmCommand> code;

public:
    explicit Compiler(std::istream &input);
    void write(std::ostream &output) const;
};