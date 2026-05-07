#include "variable.hpp"

Variable::Variable(std::string n, int t)
    : name(std::move(n)), type(t) {
    if (type == 0) {
        size = 0;
        memoryAddress = -1;
    } else if (type == 1) {
        size = 4;
    } else if (type == 2) {
        size = 8;
    }
}

Variable::Variable(std::string n, int t, std::ptrdiff_t address, bool temp)
    : Variable(std::move(n), t) {
    memoryAddress = address;
    temporary = temp;
}
