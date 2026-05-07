#pragma once

#include <cstddef>
#include <string>
#include <utility>

struct Variable {
    std::string name;
    int type = 0;
    std::size_t size = 0;
    std::ptrdiff_t memoryAddress = -1;
    bool temporary = false;

    Variable(std::string n, int t);
    Variable(std::string n, int t, std::ptrdiff_t address, bool temp);
};
