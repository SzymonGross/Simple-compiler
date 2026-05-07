#pragma once

#include <string>
#include <vector>

struct Token {
    int id = 0;
    std::vector<std::string> args;
};

Token parseLine(const std::string& line);
