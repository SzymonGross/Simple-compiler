#pragma once

#include <string>
#include <vector>

struct Token
{
    std::string name;
    std::vector<std::string> arg;

    Token(const std::string &s);
    Token();
};
