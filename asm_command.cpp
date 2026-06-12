#include "asm_command.hpp"

#include <stdexcept>
#include <iostream>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <tuple>

namespace
{
    const std::unordered_set<std::string> operands = {"+", "-", "*", "/", "%", "<<", ">>", "&&", "||", "(", ")", "==", "!=", "<=", ">=", "<", ">", "!"};

    std::unordered_map<Op, std::pair<bool, bool>> modify = {
        {Op::Mov, {1, 0}},
        {Op::Add, {1, 0}},
        {Op::Sub, {1, 0}},
        {Op::Imul, {1, 0}},

        {Op::Push, {0, 0}},
        {Op::Pop, {1, 0}},
        {Op::Ret, {0, 0}},

        {Op::Label, {0, 0}},
        {Op::Directive, {0, 0}},

        {Op::Cmp, {0, 0}},
        {Op::Test, {0, 0}},

        {Op::Jle, {0, 0}},
        {Op::Jne, {0, 0}},
        {Op::Jmp, {0, 0}},
        {Op::Je, {0, 0}},
        {Op::Jl, {0, 0}},
        {Op::Jg, {0, 0}},
        {Op::Jge, {0, 0}},

        {Op::Lea, {1, 0}},

        {Op::And, {1, 0}},
        {Op::Or, {1, 0}},
        {Op::Xor, {1, 0}},
        {Op::Sal, {1, 0}},
        {Op::Sar, {1, 0}},

        {Op::Cdq, {0, 0}},
        {Op::Cqo, {0, 0}},
        {Op::Idiv, {0, 0}},

        {Op::Sete, {1, 0}},
        {Op::Setne, {1, 0}},
        {Op::Setl, {1, 0}},
        {Op::Setle, {1, 0}},
        {Op::Setg, {1, 0}},
        {Op::Setge, {1, 0}},

        {Op::Movzx, {1, 0}},

        {Op::Call, {0, 0}},

    };

    std::unordered_map<Op, Op> negated_jump = {
        {Op::Je, Op::Jne},
        {Op::Jne, Op::Je},

        {Op::Jl, Op::Jge},
        {Op::Jge, Op::Jl},

        {Op::Jle, Op::Jg},
        {Op::Jg, Op::Jle},
    };

    std::unordered_map<Op, Op> set_to_jump = {
        {Op::Sete, Op::Je},
        {Op::Setne, Op::Jne},

        {Op::Setl, Op::Jl},
        {Op::Setle, Op::Jle},

        {Op::Setg, Op::Jg},
        {Op::Setge, Op::Jge},
    };

    std::unordered_map<std::string, std::size_t> typ_to_size = {{"logika", 1}, {"liczba", 4}, {"adres", 8}};

    std::unordered_set<std::string> data_names;
    std::size_t data_index = 1;

    std::string get_data_name(std::string fun_name)
    {
        std::string name = fun_name + "_text_" +  std::to_string(data_index);

        while (data_names.count(name))
        {
            data_index++;
            name = "text_" + fun_name + std::to_string(data_index);
        }

        data_names.insert(name);

        return name;
    }

    bool is_num(const std::string &s)
    {
        if (s.empty())
            return false;
        for (const auto &c : s)
            if (c < '0' || '9' < c)
                return false;
        return true;
    }

    std::string remove_spaces(std::string s)
    {
        for (std::size_t i = 0; i < s.size(); i++)
            while (i < s.size() && s[i] == ' ')
                s.erase(i, 1);
        return s;
    }

    bool split_expr(const std::string &expr, std::string &beg, std::string &sign, std::string &end)
    {
        std::string s = remove_spaces(expr);
        for (std::size_t i = 0; i < s.size(); i++)
        {
            std::string two;
            if (i + 1 < s.size())
                two = s.substr(i, 2);

            if (operands.count(two))
            {
                beg = s.substr(0, i);
                sign = two;
                end = s.substr(i + 2);
                return true;
            }

            std::string one = s.substr(i, 1);
            if (operands.count(one))
            {
                beg = s.substr(0, i);
                sign = one;
                end = s.substr(i + 1);
                return true;
            }
        }

        beg.clear();
        sign.clear();
        end.clear();
        return false;
    }

    std::string prefix_gen(Varbile *var)
    {
        switch (var->ele_size)
        {
        case 1:
            return "byte ptr ";
        case 2:
            return "word ptr ";
        case 4:
            return "dword ptr ";
        case 8:
            return "qword ptr ";
        }
        return "";
    }

    std::size_t prefix_idn(const std::string &s)
    {
        char c = s[0];
        switch (c)
        {
        case 'b':
            return 1;
        case 'w':
            return 2;
        case 'd':
            return 4;
        case 'q':
            return 8;
        }
        return 0;
    }

    std::string get_register(char c, std::size_t size)
    {
        switch (size)
        {
        case 1:
            return std::string() + c + "l";
        case 2:
            return std::string() + c + "x";
        case 4:
            return std::string("e") + c + "x";
        case 8:
            return std::string("r") + c + "x";
        }
        return "";
    }

    std::string get_adres(Varbile *var)
    {
        return prefix_gen(var) + "[rbp-" + std::to_string(var->addres) + "]";
    }

    std::string get_nude_adres(Varbile *var)
    {
        return "[rbp-" + std::to_string(var->addres) + "]";
    }

    std::string get_register(char c, Varbile *var)
    {
        return get_register(c, var->ele_size);
    }

    char reg_id(const std::string &s)
    {
        if (s == "al" || s == "ax" || s == "eax" || s == "rax")
            return 'a';
        if (s == "bl" || s == "bx" || s == "ebx" || s == "rbx")
            return 'b';
        if (s == "cl" || s == "cx" || s == "ecx" || s == "rcx")
            return 'c';
        if (s == "dl" || s == "dx" || s == "edx" || s == "rdx")
            return 'd';
        return '~';
    }

    std::size_t register_size(const std::string &s)
    {
        if (reg_id(s) != '~')
        {
            if (s.size() == 2 && s[1] == 'l')
                return 1;
            if (s.size() == 2 && s[1] == 'x')
                return 2;
            if (s.size() == 3 && s[0] == 'e')
                return 4;
            if (s.size() == 3 && s[0] == 'r')
                return 8;
        }

        const std::vector<std::string> ext = {"r8", "r9", "r10", "r11"};
        for (const std::string &base : ext)
        {
            if (s == base)
                return 8;
            if (s == base + "b")
                return 1;
            if (s == base + "w")
                return 2;
            if (s == base + "d")
                return 4;
        }

        const std::vector<std::tuple<std::string, std::string, std::string, std::string>> named = {
            {"rsi", "esi", "si", "sil"},
            {"rdi", "edi", "di", "dil"},
        };
        for (const auto &[r64, r32, r16, r8] : named)
        {
            if (s == r64)
                return 8;
            if (s == r32)
                return 4;
            if (s == r16)
                return 2;
            if (s == r8)
                return 1;
        }

        return 0;
    }

    bool is_register(const std::string &s)
    {
        return register_size(s) != 0;
    }

    std::string resize_register(const std::string &s, std::size_t size)
    {
        char id = reg_id(s);
        if (id != '~')
            return get_register(id, size);

        const std::vector<std::string> ext = {"r8", "r9", "r10", "r11"};
        for (const std::string &base : ext)
        {
            if (s == base || s == base + "b" || s == base + "w" || s == base + "d")
            {
                if (size == 1)
                    return base + "b";
                if (size == 2)
                    return base + "w";
                if (size == 4)
                    return base + "d";
                if (size == 8)
                    return base;
            }
        }

        const std::vector<std::tuple<std::string, std::string, std::string, std::string>> named = {
            {"rsi", "esi", "si", "sil"},
            {"rdi", "edi", "di", "dil"},
        };
        for (const auto &[r64, r32, r16, r8] : named)
            if (s == r64 || s == r32 || s == r16 || s == r8)
            {
                if (size == 1)
                    return r8;
                if (size == 2)
                    return r16;
                if (size == 4)
                    return r32;
                if (size == 8)
                    return r64;
            }

        return s;
    }

    bool is_memory(const std::string &s)
    {
        return s.find('[') != std::string::npos;
    }

    bool is_static_stack_memory(const std::string &s)
    {
        return s.find("[rbp-") != std::string::npos && s.find('+') == std::string::npos && s.find('*') == std::string::npos;
    }

    bool is_conditional_jump(Op op)
    {
        return op == Op::Je || op == Op::Jne || op == Op::Jl || op == Op::Jg || op == Op::Jle || op == Op::Jge;
    }

    bool may_change_flags(Op op)
    {
        switch (op)
        {
        case Op::Cmp:
        case Op::Test:
        case Op::Add:
        case Op::Sub:
        case Op::Imul:
        case Op::And:
        case Op::Or:
        case Op::Xor:
        case Op::Sal:
        case Op::Sar:
        case Op::Idiv:
        case Op::Call:
            return true;
        default:
            return false;
        }
    }

    bool breaks_local_flow(Op op)
    {
        return op == Op::Label || op == Op::Jmp || op == Op::Call || op == Op::Ret;
    }

    bool can_replace_rhs_memory(Op op)
    {
        switch (op)
        {
        case Op::Add:
        case Op::Sub:
        case Op::Imul:
        case Op::And:
        case Op::Or:
        case Op::Cmp:
            return true;
        default:
            return false;
        }
    }

    bool can_fold_cmp_first_operand(const std::string &src, const std::string &rhs)
    {
        if (is_num(src))
            return false;
        return !(is_memory(src) && is_memory(rhs));
    }

    bool operand_mentions_register(const std::string &operand, char id)
    {
        if (id == '~')
            return false;

        if (reg_id(operand) == id)
            return true;

        switch (id)
        {
        case 'a':
            return operand.find("rax") != std::string::npos || operand.find("eax") != std::string::npos || operand.find("ax") != std::string::npos || operand.find("al") != std::string::npos;
        case 'b':
            return operand.find("rbx") != std::string::npos || operand.find("ebx") != std::string::npos || operand.find("bx") != std::string::npos || operand.find("bl") != std::string::npos;
        case 'c':
            return operand.find("rcx") != std::string::npos || operand.find("ecx") != std::string::npos || operand.find("cx") != std::string::npos || operand.find("cl") != std::string::npos;
        case 'd':
            return operand.find("rdx") != std::string::npos || operand.find("edx") != std::string::npos || operand.find("dx") != std::string::npos || operand.find("dl") != std::string::npos;
        default:
            return false;
        }
    }

    bool reads_register(const AsmCommand &cmd, char id)
    {
        switch (cmd.op)
        {
        case Op::Mov:
        case Op::Movzx:
            return operand_mentions_register(cmd.b, id) || (is_memory(cmd.a) && operand_mentions_register(cmd.a, id));
        case Op::Lea:
            return operand_mentions_register(cmd.b, id);
        case Op::Push:
            return operand_mentions_register(cmd.a, id);
        case Op::Pop:
        case Op::Sete:
        case Op::Setne:
        case Op::Setl:
        case Op::Setle:
        case Op::Setg:
        case Op::Setge:
            return false;
        case Op::Cdq:
        case Op::Cqo:
            return id == 'a';
        case Op::Idiv:
            return id == 'a' || id == 'd' || operand_mentions_register(cmd.a, id);
        default:
            return operand_mentions_register(cmd.a, id) || operand_mentions_register(cmd.b, id);
        }
    }

    bool writes_register(const AsmCommand &cmd, char id)
    {
        if (id == '~')
            return false;

        if (cmd.op == Op::Cdq || cmd.op == Op::Cqo)
            return id == 'd';
        if (cmd.op == Op::Idiv)
            return id == 'a' || id == 'd';
        if (cmd.op == Op::Call)
            return true;

        auto it = modify.find(cmd.op);
        return it != modify.end() && it->second.first && reg_id(cmd.a) == id;
    }

    bool register_used_before_write(const std::vector<AsmCommand> &commands, char id, std::size_t beg)
    {
        for (std::size_t i = beg; i < commands.size(); i++)
        {
            const AsmCommand &cur = commands[i];
            if (breaks_local_flow(cur.op) || is_conditional_jump(cur.op))
                return true;
            if (reads_register(cur, id))
                return true;
            if (writes_register(cur, id))
                return false;
        }
        return true;
    }

    bool cmp_operands_equal(const std::pair<std::string, std::string> &lhs,
                            const std::pair<std::string, std::string> &rhs)
    {
        return lhs.first == rhs.first && lhs.second == rhs.second;
    }

    void load_index_to_rcx(std::vector<AsmCommand> &out, Varbile *idx)
    {
        if (idx->ele_size == 8)
            out.emplace_back(Op::Mov, "rcx", get_adres(idx));
        else if (idx->ele_size == 4)
            out.emplace_back(Op::Mov, "ecx", get_adres(idx));
        else
            out.emplace_back(Op::Movzx, "ecx", get_adres(idx));
    }

    void load_printf_arg(std::vector<AsmCommand> &out, const std::string &target, const std::string &value, Varbile *var)
    {
        if (is_num(value))
        {
            out.emplace_back(Op::Mov, target, value);
            return;
        }

        if (var->ele_size == 8)
            out.emplace_back(Op::Mov, target, get_adres(var));
        else if (var->ele_size == 4)
            out.emplace_back(Op::Mov, resize_register(target, 4), get_adres(var));
        else
            out.emplace_back(Op::Movzx, resize_register(target, 4), get_adres(var));
    }

    struct StackSlot
    {
        int old_end = 0;
        int size = 0;
        int new_end = 0;
    };

    bool stack_offset_span(const std::string &operand, std::size_t &num_begin, std::size_t &num_len, int &offset)
    {
        if (!is_static_stack_memory(operand))
            return false;

        std::size_t rbp = operand.find("[rbp-");
        if (rbp == std::string::npos)
            return false;

        num_begin = rbp + 5;
        std::size_t pos = num_begin;
        while (pos < operand.size() && operand[pos] >= '0' && operand[pos] <= '9')
            pos++;

        if (pos == num_begin || pos >= operand.size() || operand[pos] != ']')
            return false;

        num_len = pos - num_begin;
        offset = std::stoi(operand.substr(num_begin, num_len));
        return true;
    }

    bool stack_offset_value(const std::string &operand, int &offset)
    {
        std::size_t num_begin = 0, num_len = 0;
        return stack_offset_span(operand, num_begin, num_len, offset);
    }

    int align_stack_size(int used_size)
    {
        return (used_size + 32 + 15) / 16 * 16;
    }
}

std::string AsmCommand::emit() const
{
    switch (op)
    {
    case Op::Mov:
        return "mov " + a + ", " + b;
    case Op::Add:
        return "add " + a + ", " + b;
    case Op::Sub:
        return "sub " + a + ", " + b;
    case Op::Imul:
        return "imul " + a + ", " + b;
    case Op::Push:
        return "push " + a;
    case Op::Pop:
        return "pop " + a;
    case Op::Ret:
        return "ret\n";
    case Op::Label:
        return a + ":";
    case Op::Directive:
        return a + "";
    case Op::Cmp:
        return "cmp " + a + ", " + b;
    case Op::Jle:
        return "jle " + a + "\n";
    case Op::Jne:
        return "jne " + a + "\n";
    case Op::Jmp:
        return "jmp " + a + "\n";
    case Op::Je:
        return "je " + a + "\n";
    case Op::Jl:
        return "jl " + a + "\n";
    case Op::Jg:
        return "jg " + a + "\n";
    case Op::Jge:
        return "jge " + a + "\n";
    case Op::Lea:
        return "lea " + a + ", " + b;
    case Op::And:
        return "and " + a + ", " + b;
    case Op::Or:
        return "or " + a + ", " + b;
    case Op::Xor:
        return "xor " + a + ", " + b;
    case Op::Sal:
        return "sal " + a + ", " + b;
    case Op::Sar:
        return "sar " + a + ", " + b;
    case Op::Cdq:
        return "cdq";
    case Op::Cqo:
        return "cqo";
    case Op::Idiv:
        return "idiv " + a;
    case Op::Sete:
        return "sete " + a;
    case Op::Setne:
        return "setne " + a;
    case Op::Setl:
        return "setl " + a;
    case Op::Setle:
        return "setle " + a;
    case Op::Setg:
        return "setg " + a;
    case Op::Setge:
        return "setge " + a;
    case Op::Movzx:
        return "movzx " + a + ", " + b;
    case Op::Call:
        return "call " + a + "\n";
    case Op::Test:
        return "test " + a + ", " + b;
    case Op::Asciz:
        return ".asciz " + a;
    }

    return {};
}

std::pair<std::vector<AsmCommand>, std::vector<AsmCommand>> Generator::gen_command(Tree::Node *node)
{
    std::pair<std::vector<AsmCommand>, std::vector<AsmCommand>> res;

    auto require_mem = [&](const std::string &name) -> Varbile *
    {
        auto it = mem.find(name);
        if (it == mem.end() || it->second == nullptr)
            throw std::runtime_error("Nieznana zmienna: " + name);
        return it->second;
    };

    if (node->name == "ustaw")
    {
        if (node->arg.size() < 2 || (!mem.count(node->arg[0]) && !is_register(node->arg[0])))
            throw std::runtime_error("Bledna komenda ustaw");

        Varbile *dst = mem.count(node->arg[0]) ? mem[node->arg[0]] : nullptr;
        bool dst_is_register = dst == nullptr;
        std::size_t dst_size = dst_is_register ? register_size(node->arg[0]) : dst->ele_size;

        auto require_var = [&](const std::string &name) -> Varbile *
        {
            return require_mem(name);
        };

        auto operand_size = [&](const std::string &operand, std::size_t fallback)
        {
            if (is_register(operand))
                return register_size(operand);

            auto it = mem.find(operand);
            if (it != mem.end())
                return it->second->ele_size;
            return fallback;
        };

        auto operation_size = [&](const std::string &left, const std::string &right)
        {
            std::size_t left_size = operand_size(left, dst_size);
            std::size_t right_size = operand_size(right, dst_size);
            return std::max(dst_size, std::max(left_size, right_size));
        };

        auto load_operand = [&](const std::string &operand, char reg, std::size_t size)
        {
            if (is_num(operand))
            {
                res.first.emplace_back(Op::Mov, get_register(reg, size), operand);
                return;
            }

            if (is_register(operand))
            {
                std::size_t src_size = register_size(operand);
                std::string dst_reg = get_register(reg, size);

                if (src_size == size)
                {
                    std::string src_reg = resize_register(operand, size);
                    if (src_reg != dst_reg)
                        res.first.emplace_back(Op::Mov, dst_reg, src_reg);
                }
                else if (src_size < size)
                {
                    if (src_size == 4)
                        res.first.emplace_back(Op::Mov, get_register(reg, 4), resize_register(operand, 4));
                    else
                        res.first.emplace_back(Op::Movzx, dst_reg, resize_register(operand, src_size));
                }
                else
                {
                    res.first.emplace_back(Op::Mov, dst_reg, resize_register(operand, size));
                }

                return;
            }

            Varbile *src = require_var(operand);
            if (src->ele_size == size)
                res.first.emplace_back(Op::Mov, get_register(reg, size), get_adres(src));
            else if (src->ele_size < size)
            {
                if (src->ele_size == 4 && size == 8)
                    res.first.emplace_back(Op::Mov, get_register(reg, 4), get_adres(src));
                else
                    res.first.emplace_back(Op::Movzx, get_register(reg, size), get_adres(src));
            }
            else
            {
                res.first.emplace_back(Op::Mov, get_register(reg, src), get_adres(src));
                res.first.emplace_back(Op::Mov, get_register(reg, size), get_register(reg, src));
            }
        };

        auto load_operand_to_register = [&](const std::string &target, const std::string &operand, std::size_t size)
        {
            std::string dst_reg = resize_register(target, size);
            if (is_num(operand))
            {
                res.first.emplace_back(Op::Mov, dst_reg, operand);
                return;
            }

            if (is_register(operand))
            {
                std::size_t src_size = register_size(operand);
                if (src_size == size)
                {
                    std::string src_reg = resize_register(operand, size);
                    if (src_reg != dst_reg)
                        res.first.emplace_back(Op::Mov, dst_reg, src_reg);
                }
                else if (src_size < size)
                {
                    if (src_size == 4)
                        res.first.emplace_back(Op::Mov, resize_register(target, 4), resize_register(operand, 4));
                    else
                        res.first.emplace_back(Op::Movzx, dst_reg, resize_register(operand, src_size));
                }
                else
                {
                    res.first.emplace_back(Op::Mov, dst_reg, resize_register(operand, size));
                }
                return;
            }

            Varbile *src = require_var(operand);
            if (src->ele_size == size)
                res.first.emplace_back(Op::Mov, dst_reg, get_adres(src));
            else if (src->ele_size < size)
            {
                if (src->ele_size == 4 && size == 8)
                    res.first.emplace_back(Op::Mov, resize_register(target, 4), get_adres(src));
                else
                    res.first.emplace_back(Op::Movzx, dst_reg, get_adres(src));
            }
            else
                res.first.emplace_back(Op::Mov, resize_register(target, src->ele_size), get_adres(src));
        };

        auto alu_operand = [&](const std::string &operand, std::size_t size)
        {
            if (is_num(operand))
                return operand;

            if (is_register(operand))
            {
                if (register_size(operand) == size)
                    return resize_register(operand, size);

                load_operand(operand, 'c', size);
                return get_register('c', size);
            }

            Varbile *src = require_var(operand);
            if (src->ele_size == size)
                return get_adres(src);

            load_operand(operand, 'c', size);
            return get_register('c', size);
        };

        auto store_from_a = [&]()
        {
            if (dst_is_register)
            {
                std::string dst_reg = resize_register(node->arg[0], dst_size);
                std::string src_reg = get_register('a', dst_size);
                if (dst_reg != src_reg)
                    res.first.emplace_back(Op::Mov, dst_reg, src_reg);
            }
            else
                res.first.emplace_back(Op::Mov, get_adres(dst), get_register('a', dst));
        };

        auto store_bool_from_al = [&]()
        {
            if (dst_is_register)
            {
                if (dst_size == 1)
                {
                    std::string dst_reg = resize_register(node->arg[0], 1);
                    if (dst_reg != "al")
                        res.first.emplace_back(Op::Mov, dst_reg, "al");
                }
                else
                    res.first.emplace_back(Op::Movzx, resize_register(node->arg[0], dst_size), "al");
            }
            else if (dst->ele_size == 1)
                res.first.emplace_back(Op::Mov, get_adres(dst), "al");
            else
            {
                res.first.emplace_back(Op::Movzx, get_register('a', dst), "al");
                store_from_a();
            }
        };

        auto setcc_for = [&](const std::string &sign)
        {
            if (sign == "==")
                return Op::Sete;
            if (sign == "!=")
                return Op::Setne;
            if (sign == "<")
                return Op::Setl;
            if (sign == "<=")
                return Op::Setle;
            if (sign == ">")
                return Op::Setg;
            if (sign == ">=")
                return Op::Setge;
            throw std::runtime_error("Nieznana operacja porownania: " + sign);
        };

        std::string expr = remove_spaces(node->arg[1]);
        std::string beg, sign, end;

        if (!split_expr(expr, beg, sign, end))
        {
            if (is_num(expr))
            {
                if (dst_is_register)
                    res.first.emplace_back(Op::Mov, resize_register(node->arg[0], dst_size), expr);
                else
                    res.first.emplace_back(Op::Mov, get_adres(dst), expr);
            }
            else
            {
                if (dst_is_register)
                    load_operand_to_register(node->arg[0], expr, dst_size);
                else
                {
                    load_operand(expr, 'a', dst_size);
                    store_from_a();
                }
            }
        }
        else if (sign == "!")
        {
            std::size_t size = operand_size(end, dst_size);
            load_operand(end, 'a', size);
            res.first.emplace_back(Op::Cmp, get_register('a', size), "0");
            res.first.emplace_back(Op::Sete, "al");
            store_bool_from_al();
        }
        else if (sign == "==" || sign == "!=" || sign == "<=" || sign == ">=" || sign == "<" || sign == ">")
        {
            std::size_t size = operation_size(beg, end);
            load_operand(beg, 'a', size);
            if (is_num(end))
                res.first.emplace_back(Op::Cmp, get_register('a', size), end);
            else
            {
                load_operand(end, 'c', size);
                res.first.emplace_back(Op::Cmp, get_register('a', size), get_register('c', size));
            }
            res.first.emplace_back(setcc_for(sign), "al");
            store_bool_from_al();
        }
        else if (sign == "&&" || sign == "||")
        {
            std::size_t left_size = operand_size(beg, dst_size);
            std::size_t right_size = operand_size(end, dst_size);

            load_operand(beg, 'a', left_size);
            res.first.emplace_back(Op::Cmp, get_register('a', left_size), "0");
            res.first.emplace_back(Op::Setne, "al");

            load_operand(end, 'c', right_size);
            res.first.emplace_back(Op::Cmp, get_register('c', right_size), "0");
            res.first.emplace_back(Op::Setne, "cl");

            res.first.emplace_back(sign == "&&" ? Op::And : Op::Or, "al", "cl");
            store_bool_from_al();
        }
        else if (sign == "+" || sign == "-" || sign == "*")
        {
            std::size_t size = operation_size(beg, end);
            load_operand(beg, 'a', size);
            std::string rhs = alu_operand(end, size);

            if (sign == "+")
                res.first.emplace_back(Op::Add, get_register('a', size), rhs);
            else if (sign == "-")
                res.first.emplace_back(Op::Sub, get_register('a', size), rhs);
            else
                res.first.emplace_back(Op::Imul, get_register('a', size), rhs);

            store_from_a();
        }
        else if (sign == "/" || sign == "%")
        {
            std::size_t size = operation_size(beg, end);
            if (size != 4 && size != 8)
                throw std::runtime_error("Dzielenie wymaga typu liczba");

            load_operand(beg, 'a', size);
            res.first.emplace_back(size == 4 ? Op::Cdq : Op::Cqo);

            std::string rhs;
            if (is_num(end))
            {
                res.first.emplace_back(Op::Mov, get_register('c', size), end);
                rhs = get_register('c', size);
            }
            else
                rhs = alu_operand(end, size);

            res.first.emplace_back(Op::Idiv, rhs);
            if (sign == "%")
                res.first.emplace_back(Op::Mov, get_register('a', size), get_register('d', size));

            store_from_a();
        }
        else if (sign == "<<" || sign == ">>")
        {
            std::size_t size = operation_size(beg, end);
            load_operand(beg, 'a', size);
            if (is_num(end))
                res.first.emplace_back(sign == "<<" ? Op::Sal : Op::Sar, get_register('a', size), end);
            else
            {
                load_operand(end, 'c', operand_size(end, 4));
                res.first.emplace_back(sign == "<<" ? Op::Sal : Op::Sar, get_register('a', size), "cl");
            }
            store_from_a();
        }
        else
            throw std::runtime_error("Nieznana operacja: " + sign);
    }
    else if (node->name == "zapisz")
    {
        if (is_num(node->arg[0]))
        {
            std::string full_adres;

            if (is_num(node->arg[2]))
            {
                full_adres = prefix_gen(mem[node->arg[1]]) + "[rbp-" + std::to_string(mem[node->arg[1]]->addres - mem[node->arg[1]]->ele_size * stoi(node->arg[2])) + "]";
            }
            else
            {
                load_index_to_rcx(res.first, require_mem(node->arg[2]));
                res.first.emplace_back(Op::Lea, "rdx", get_nude_adres(mem[node->arg[1]]));
                full_adres = prefix_gen(mem[node->arg[1]]) + "[rdx + rcx*" + std::to_string(mem[node->arg[1]]->ele_size) + "]";
            }

            res.first.emplace_back(Op::Mov, full_adres, node->arg[0]);
        }

        else
        {
            res.first.emplace_back(Op::Mov, get_register('a', mem[node->arg[1]]), get_adres(mem[node->arg[0]]));
            std::string full_adres;

            if (is_num(node->arg[2]))
            {
                full_adres = prefix_gen(mem[node->arg[1]]) + "[rbp-" + std::to_string(mem[node->arg[1]]->addres - mem[node->arg[1]]->ele_size * stoi(node->arg[2])) + "]";
            }
            else
            {
                load_index_to_rcx(res.first, require_mem(node->arg[2]));
                res.first.emplace_back(Op::Lea, "rdx", get_nude_adres(mem[node->arg[1]]));
                full_adres = prefix_gen(mem[node->arg[1]]) + "[rdx + rcx*" + std::to_string(mem[node->arg[1]]->ele_size) + "]";
            }

            res.first.emplace_back(Op::Mov, full_adres, get_register('a', mem[node->arg[1]]));
        }
    }
    else if (node->name == "wczytaj_ram")
    {
        Varbile *dst = mem[node->arg[0]];
        Varbile *arr = mem[node->arg[1]];

        std::string full_adres;

        if (is_num(node->arg[2]))
        {
            full_adres = prefix_gen(arr) + "[rbp-" + std::to_string(arr->addres - arr->ele_size * stoi(node->arg[2])) + "]";
        }
        else
        {
            load_index_to_rcx(res.first, require_mem(node->arg[2]));
            res.first.emplace_back(Op::Lea, "rdx", get_nude_adres(arr));

            full_adres = prefix_gen(arr) + "[rdx + rcx*" + std::to_string(arr->ele_size) + "]";
        }

        res.first.emplace_back(Op::Mov, get_register('a', arr), full_adres);
        res.first.emplace_back(Op::Mov, get_adres(dst), get_register('a', arr));
    }
    else if (node->name == "punkt")
    {
        res.first.emplace_back(Op::Label, node->arg[0]);
    }
    else if (node->name == "idzjezeli")
    {
        if (is_num(node->arg[0]))
        {
            res.first.emplace_back(Op::Mov, "al", node->arg[0]);
            res.first.emplace_back(Op::Cmp, "al", "0");
        }
        else
            res.first.emplace_back(Op::Cmp, get_adres(require_mem(node->arg[0])), "0");
        res.first.emplace_back(Op::Jne, node->arg[1]);
    }
    else if (node->name == "idz")
    {
        res.first.emplace_back(Op::Jmp, node->arg[0]);
    }
    else if (node->name == "zakoncz")
    {
        auto enclosing_return_size = [&]()
        {
            for (Tree::Node *cur = node; cur != nullptr; cur = cur->parent)
                if (cur->name == "enter_point" && !cur->arg.empty() && typ_to_size.count(cur->arg[0]))
                    return typ_to_size[cur->arg[0]];
            return static_cast<std::size_t>(4);
        };

        std::size_t ret_size = is_num(node->arg[0]) ? enclosing_return_size() : require_mem(node->arg[0])->ele_size;
        if (is_num(node->arg[0]))
            res.first.emplace_back(Op::Mov, get_register('a', ret_size), node->arg[0]);
        else
            res.first.emplace_back(Op::Mov, get_register('a', ret_size), get_adres(require_mem(node->arg[0])));
    }
    else if (node->name == "wywolaj")
    {
        res.first.emplace_back(Op::Call, node->arg[0]);
    }
    else if (node->name == "wypisz")
    {
        contains_printf = true;

        std::string data_name = get_data_name(body[0].a);
        data.emplace_back(Op::Label, data_name);
        data.emplace_back(Op::Asciz, node->arg[0]);

        body.emplace_back(Op::Lea, "rcx", "[rip + " + data_name + "]");

        auto &v = node->arg;

        std::vector<std::string> reg = {"rdx", "r8", "r9"};

        if (v.size() > 4)
            throw std::runtime_error("Za dużo argumentów w wypisz");

        for (std::size_t i = 1; i < v.size(); i++)
        {
            Varbile *var = is_num(v[i]) ? nullptr : require_mem(v[i]);
            load_printf_arg(body, reg[i - 1], v[i], var);
        }

        body.emplace_back(Op::Xor, "eax", "eax");
        body.emplace_back(Op::Call, "printf");
    }
    else if (node->name == "wczytaj")
    {
        contains_scanf = true;

        std::string data_name = get_data_name(body[0].a);
        data.emplace_back(Op::Label, data_name);
        data.emplace_back(Op::Asciz, node->arg[0]);

        body.emplace_back(Op::Lea, "rcx", "[rip + " + data_name + "]");

        auto &v = node->arg;

        std::vector<std::string> reg = {"rdx", "r8", "r9"};

        if (v.size() > 4)
            throw std::runtime_error("Za dużo argumentów w wczytaj");

        for (std::size_t i = 1; i < v.size(); i++)
        {
            std::size_t adr = mem[v[i]]->addres;
            body.emplace_back(Op::Lea, reg[i - 1], "[rbp-" + std::to_string(adr) + "]");
        }

        body.emplace_back(Op::Xor, "eax", "eax");
        body.emplace_back(Op::Call, "scanf");
    }
    else if (node->name == "enter_point")
    {
        std::string lab;
        if (node->arg.size() > 1)
            lab = node->arg[1];
        else
            lab = node->arg[0];

        res.first.emplace_back(Op::Label, lab);

        int stack_size = static_cast<int>(stack.size()) - 1 + 32;
        stack_size = (stack_size + 15) / 16 * 16;

        res.first.emplace_back(Op::Push, "rbp");
        res.first.emplace_back(Op::Mov, "rbp", "rsp");
        res.first.emplace_back(Op::Sub, "rsp", std::to_string(stack_size));

        res.second.emplace_back(Op::Add, "rsp", std::to_string(stack_size));
        res.second.emplace_back(Op::Pop, "rbp");
        res.second.emplace_back(Op::Ret);
    }
    else if (node->name != "stworz" && node->name != "usun")
        throw std::runtime_error("Nieznana komenda: " + node->name);

    return res;
}

void Generator::generator_step(Tree::Node *node)
{
    std::pair<std::vector<AsmCommand>, std::vector<AsmCommand>> temp = move(gen_command(node));

    for (AsmCommand &a : temp.first)
        body.push_back(a);

    for (const auto &n : node->child)
        generator_step(n);

    for (AsmCommand &a : temp.second)
        body.push_back(a);
}

void Generator::mem_aloc(Tree::Node *node)
{
    if (node->name == "stworz")
    {
        if (mem.count(node->arg.back()))
            throw std::runtime_error("Ponowna deklaracja: " + node->arg.back());

        std::string type = node->arg[0];
        if (node->arg[0] == "tablica")
            type += "_" + node->arg[1];

        Varbile *var = new Varbile(node->arg.back(), type);

        std::size_t size = 1;
        if (node->arg[0] == "tablica")
        {
            size = stoi(node->arg[2]);
            size *= typ_to_size[node->arg[1]];
            var->ele_size = typ_to_size[node->arg[1]];
        }
        else
        {
            size = typ_to_size[node->arg[0]];
            var->ele_size = typ_to_size[node->arg[0]];
        }

        var->size = size;
        int i = size;

        while (true)
        {
            while (stack.size() <= i)
                stack.push_back(0);

            bool is = 0;
            for (int j = i; i - size < j; j--)
                if (stack[j])
                {
                    is = 1;
                    i += std::max(i + static_cast<int>(size) - j, 1);
                    break;
                }

            if (!is)
                break;
        }
        var->addres = i;

        for (std::size_t j = var->addres; j + var->size > var->addres; j--)
            stack[j] = 1;

        mem[node->arg.back()] = var;
    }
    else if (node->name == "usun")
    {
        const std::string &name = node->arg.back();

        if (!mem.count(name))
            throw std::runtime_error("Ponowne usunięcie: " + name);

        Varbile *var = mem[name];

        for (std::size_t j = var->addres; j + var->size > var->addres; j--)
            stack[j] = 0;
    }

    for (const auto &n : node->child)
        mem_aloc(n);
}

bool Generator::worth_storing(const std::string &s, const std::size_t beg) const
{
    std::unordered_map<std::string, std::size_t> labels;
    for (std::size_t i = 0; i < body.size(); i++)
        if (body[i].op == Op::Label)
            labels[body[i].a] = i;

    auto reads_slot = [&](const AsmCommand &cur)
    {
        if (cur.b == s)
            return true;
        if (cur.a != s)
            return false;

        switch (cur.op)
        {
        case Op::Add:
        case Op::Sub:
        case Op::Imul:
        case Op::And:
        case Op::Or:
        case Op::Xor:
        case Op::Sal:
        case Op::Sar:
        case Op::Cmp:
        case Op::Test:
        case Op::Push:
            return true;
        default:
            return false;
        }
    };

    auto writes_slot = [&](const AsmCommand &cur)
    {
        auto it = modify.find(cur.op);
        return cur.a == s && it != modify.end() && it->second.first;
    };

    std::vector<std::size_t> pending = {beg + 1};
    std::unordered_set<std::size_t> visited;

    while (!pending.empty())
    {
        std::size_t i = pending.back();
        pending.pop_back();

        if (i >= body.size() || visited.count(i))
            continue;
        visited.insert(i);

        const AsmCommand &cur = body[i];
        if (cur.op != Op::Directive && cur.op != Op::Label)
            if (!modify.count(cur.op))
                throw std::runtime_error("Nieznana komenda: " + cur.emit());

        if (reads_slot(cur))
            return true;
        if (writes_slot(cur))
            continue;

        if (cur.op == Op::Call)
        {
            pending.push_back(i + 1);
            continue;
        }
        if (cur.op == Op::Ret)
            continue;

        if (cur.op == Op::Jmp)
        {
            auto it = labels.find(cur.a);
            if (it == labels.end())
                return true;
            pending.push_back(it->second);
            continue;
        }

        if (is_conditional_jump(cur.op))
        {
            auto it = labels.find(cur.a);
            if (it == labels.end())
                return true;
            pending.push_back(it->second);
            pending.push_back(i + 1);
            continue;
        }

        pending.push_back(i + 1);
    }
    return false;
}

bool Generator::Piphole_opt()
{
    bool changed = 0;
    std::unordered_map<char, std::string> registers;
    bool known_cmp = false;
    std::pair<std::string, std::string> last_cmp;

    auto clear_known_cmp = [&]()
    {
        known_cmp = false;
        last_cmp = {};
    };

    auto find = [&](const std::string &s)
    { for (const auto &[p, v] : registers) if (v == s) return p; return '~'; };

    auto find_except = [&](const std::string &s, char except)
    { for (const auto &[p, v] : registers) if (p != except && v == s) return p; return '~'; };

    auto value_of = [&](const std::string &s)
    {
        char id = reg_id(s);
        if (id != '~' && registers.count(id))
            return registers[id];
        return s;
    };

    auto erase_value = [&](const std::string &s)
    {
        for (auto it = registers.begin(); it != registers.end();)
        {
            if (it->second == s)
                it = registers.erase(it);
            else
                ++it;
        }
    };

    auto cmp_uses_changed_memory = [&](const std::string &s)
    {
        return known_cmp && is_memory(s) && (last_cmp.first == s || last_cmp.second == s);
    };

    auto cmp_uses_any_memory = [&]()
    {
        return known_cmp && (is_memory(last_cmp.first) || is_memory(last_cmp.second));
    };

    auto cmp_uses_changed_register = [&](char id)
    {
        if (!known_cmp || id == '~')
            return false;
        return reg_id(last_cmp.first) == id || reg_id(last_cmp.second) == id;
    };

    auto register_for_memory = [&](const std::string &s, char except = '~')
    {
        char id = find_except(s, except);
        if (id == '~' && except == '~')
            id = find(s);
        if (id == '~')
            return std::string();
        return get_register(id, prefix_idn(s));
    };

    auto replace_rhs_memory = [&](AsmCommand &cmd)
    {
        if (!can_replace_rhs_memory(cmd.op) || !is_static_stack_memory(cmd.b))
            return false;

        std::string reg = register_for_memory(cmd.b);
        if (reg.empty())
            return false;

        cmd.b = reg;
        return true;
    };

    auto replace_cmp_memory = [&](AsmCommand &cmd)
    {
        bool local_changed = false;
        if (is_static_stack_memory(cmd.a))
        {
            std::string reg = register_for_memory(cmd.a);
            if (!reg.empty())
            {
                cmd.a = reg;
                local_changed = true;
            }
        }
        if (is_static_stack_memory(cmd.b))
            local_changed |= replace_rhs_memory(cmd);
        return local_changed;
    };

    auto remember_register_write = [&](const AsmCommand &cmd)
    {
        char dst = reg_id(cmd.a);
        if (dst == '~')
            return;

        if (cmp_uses_changed_register(dst))
            clear_known_cmp();

        registers.erase(dst);
        if (cmd.op == Op::Mov && is_static_stack_memory(cmd.b))
            registers[dst] = cmd.b;
        else if (cmd.op == Op::Mov)
        {
            char src = reg_id(cmd.b);
            if (src != '~' && registers.count(src))
                registers[dst] = registers[src];
        }
    };

    auto forget_implicit_register_writes = [&](const AsmCommand &cmd)
    {
        for (char id : {'a', 'b', 'c', 'd'})
        {
            if (!writes_register(cmd, id))
                continue;

            if (cmp_uses_changed_register(id))
                clear_known_cmp();
            registers.erase(id);
        }
    };

    auto remember_memory_write = [&](const AsmCommand &cmd)
    {
        if (!is_memory(cmd.a))
            return;

        if (!is_static_stack_memory(cmd.a))
        {
            registers.clear();
            if (cmp_uses_any_memory())
                clear_known_cmp();
            return;
        }

        erase_value(cmd.a);
        if (cmp_uses_changed_memory(cmd.a))
            clear_known_cmp();

        char src = reg_id(cmd.b);
        if (src != '~')
            registers[src] = cmd.a;
    };

    for (std::size_t i = 0; i < body.size(); i++)
    {
        AsmCommand &cur = body[i];
        if (cur.op == Op::Label)
        {
            registers.clear();
            clear_known_cmp();
            continue;
        }

        if (cur.op == Op::Mov && is_register(cur.a))
        {
            char dst = reg_id(cur.a);

            if (is_static_stack_memory(cur.b) && registers.count(dst) && registers[dst] == cur.b)
            {
                body.erase(body.begin() + i);
                i--;
                changed = 1;
                continue;
            }

            if (!register_used_before_write(body, dst, i + 1))
            {
                body.erase(body.begin() + i);
                i--;
                changed = 1;
                continue;
            }
        }

        if (cur.op == Op::Mov && is_register(cur.a) && is_register(cur.b) && i + 1 < body.size())
        {
            AsmCommand &next = body[i + 1];
            if (next.op == Op::Mov && is_static_stack_memory(next.a) && next.b == cur.a && !register_used_before_write(body, reg_id(cur.a), i + 2))
            {
                next.b = cur.b;
                body.erase(body.begin() + i);
                i--;
                changed = 1;
                continue;
            }
        }

        if (cur.op == Op::Mov && is_register(cur.a) && is_static_stack_memory(cur.b))
        {
            std::string reg = register_for_memory(cur.b, reg_id(cur.a));
            if (!reg.empty())
            {
                cur.b = reg;
                changed = 1;
            }
        }
        else if (cur.op == Op::Cmp)
        {
            changed |= replace_cmp_memory(cur);
        }
        else
        {
            changed |= replace_rhs_memory(cur);
        }

        if (cur.op == Op::Cmp && is_register(cur.a) && i > 0)
        {
            const AsmCommand &prev = body[i - 1];
            if (prev.op == Op::Mov && prev.a == cur.a && can_fold_cmp_first_operand(prev.b, cur.b) && !operand_mentions_register(prev.b, reg_id(cur.a)))
            {
                cur.a = prev.b;
                changed = 1;
            }
        }

        if (known_cmp && set_to_jump.count(cur.op) && i + 2 < body.size())
        {
            AsmCommand next1 = body[i + 1];
            AsmCommand next2 = body[i + 2];

            bool zero_check = (next1.op == Op::Cmp && next1.b == "0" && cur.a == next1.a) ||
                              (next1.op == Op::Test && next1.a == cur.a && next1.b == cur.a);

            if (zero_check && next2.op == Op::Jne)
            {
                Op opp = set_to_jump[cur.op];

                cur = AsmCommand(opp, next2.a);
                body.erase(body.begin() + i + 1);
                body.erase(body.begin() + i + 1);

                changed = 1;
                continue;
            }
        }

        if (cur.op == Op::Cmp)
        {
            std::pair<std::string, std::string> cur_cmp = {value_of(cur.a), value_of(cur.b)};
            if (known_cmp && cmp_operands_equal(last_cmp, cur_cmp))
            {
                body.erase(body.begin() + i);
                i--;
                changed = 1;
                continue;
            }

            known_cmp = true;
            last_cmp = cur_cmp;

            if (is_register(cur.a) && cur.b == "0")
            {
                cur.op = Op::Test;
                cur.b = cur.a;
                changed = 1;
            }
        }
        else if (cur.op == Op::Test && cur.a == cur.b && is_register(cur.a))
        {
            known_cmp = true;
            last_cmp = {value_of(cur.a), "0"};
        }
        else if (may_change_flags(cur.op))
            clear_known_cmp();

        if (cur.op == Op::Mov && is_static_stack_memory(cur.a))
            if (!worth_storing(cur.a, i))
            {
                body.erase(body.begin() + i);
                i--;
                changed = 1;
                continue;
            }

        if (cur.op == Op::Mov && is_memory(cur.a))
            remember_memory_write(cur);
        else if (cur.op == Op::Mov && is_register(cur.a))
            remember_register_write(cur);
        else if (writes_register(cur, reg_id(cur.a)))
            remember_register_write(cur);
        else
            forget_implicit_register_writes(cur);

        if (breaks_local_flow(cur.op))
        {
            registers.clear();
            clear_known_cmp();
        }

        if (cur.op == Op::Mov && cur.a.size() <= 3 && cur.b.size() <= 3)
        {
            std::string reg = cur.b;

            std::size_t j = 1;

            while (body[i - j].a == reg)
                j++;
            j--;

            if (j && body[i - j].op == Op::Mov)
            {
                for (; j > 0; j--)
                    body[i - j].a = cur.a;
                body.erase(body.begin() + i);
                i--;
            }
        }
    }
    return changed;
}

void Generator::compact_stack_offsets()
{
    std::vector<int> used_offsets;
    auto collect_offset = [&](const std::string &operand)
    {
        int offset = 0;
        if (stack_offset_value(operand, offset))
            used_offsets.push_back(offset);
    };

    for (const AsmCommand &cmd : body)
    {
        collect_offset(cmd.a);
        collect_offset(cmd.b);
    }

    if (used_offsets.empty())
    {
        int stack_size = align_stack_size(0);
        for (AsmCommand &cmd : body)
            if ((cmd.op == Op::Sub || cmd.op == Op::Add) && cmd.a == "rsp")
                cmd.b = std::to_string(stack_size);
        return;
    }

    std::vector<StackSlot> slots;
    auto add_slot = [&](int old_end, int size)
    {
        if (old_end <= 0 || size <= 0)
            return;

        for (StackSlot &slot : slots)
        {
            if (slot.old_end == old_end)
            {
                slot.size = std::max(slot.size, size);
                return;
            }
        }

        StackSlot slot;
        slot.old_end = old_end;
        slot.size = size;
        slots.push_back(slot);
    };

    for (int offset : used_offsets)
    {
        Varbile *best = nullptr;
        for (const auto &entry : mem)
        {
            Varbile *var = entry.second;
            if (var == nullptr || var->size == 0)
                continue;

            int old_end = static_cast<int>(var->addres);
            int size = static_cast<int>(var->size);
            if (old_end - size < offset && offset <= old_end)
                if (best == nullptr || var->size < best->size)
                    best = var;
        }

        if (best != nullptr)
            add_slot(static_cast<int>(best->addres), static_cast<int>(best->size));
        else
            add_slot(offset, 1);
    }

    std::sort(slots.begin(), slots.end(), [](const StackSlot &lhs, const StackSlot &rhs)
              { return lhs.old_end < rhs.old_end; });

    int used_size = 0;
    for (StackSlot &slot : slots)
    {
        used_size += slot.size;
        slot.new_end = used_size;
    }

    auto remap_offset = [&](int offset)
    {
        const StackSlot *best = nullptr;
        for (const StackSlot &slot : slots)
            if (slot.old_end - slot.size < offset && offset <= slot.old_end)
                if (best == nullptr || slot.size < best->size)
                    best = &slot;

        if (best == nullptr)
            return offset;
        return best->new_end - (best->old_end - offset);
    };

    auto replace_offset = [&](std::string &operand)
    {
        std::size_t num_begin = 0, num_len = 0;
        int offset = 0;
        if (!stack_offset_span(operand, num_begin, num_len, offset))
            return;

        int new_offset = remap_offset(offset);
        operand.replace(num_begin, num_len, std::to_string(new_offset));
    };

    for (AsmCommand &cmd : body)
    {
        replace_offset(cmd.a);
        replace_offset(cmd.b);
    }

    int stack_size = align_stack_size(used_size);
    for (AsmCommand &cmd : body)
        if ((cmd.op == Op::Sub || cmd.op == Op::Add) && cmd.a == "rsp")
            cmd.b = std::to_string(stack_size);
}

Generator::Generator(Tree tree)
{
    mem_aloc(tree.get_beg());
    generator_step(tree.get_beg());
}
