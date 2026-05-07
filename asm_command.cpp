#include "asm_command.hpp"

std::string AsmCommand::emit() const {
    switch (op) {
        case Op::Mov:      return "mov " + a + ", " + b;
        case Op::Add:      return "add " + a + ", " + b;
        case Op::Sub:      return "sub " + a + ", " + b;
        case Op::Imul:     return "imul " + a + ", " + b;
        case Op::Push:     return "push " + a;
        case Op::Pop:      return "pop " + a;
        case Op::Ret:      return "ret";
        case Op::Label:    return a;
        case Op::Directive:return a;
        case Op::Cmp:      return "cmp " + a + ", " + b;
        case Op::Jle:      return "jle " + a;
        case Op::Jmp:      return "jmp " + a;
    }
    return {};
}
