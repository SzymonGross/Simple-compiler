#include "tree.hpp"
#include "token.hpp"

#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <limits>
#include <stdexcept>
#include <cctype>
#include <algorithm>

namespace
{
    std::unordered_map<std::string, long long> constants;
    std::unordered_set<std::string> var_names;
    std::unordered_map<std::string, std::string> var_types;
    std::unordered_set<std::string> stop_names;

    const std::unordered_set<std::string> operands = {"+", "-", "*", "/", "%", "<<", ">>", "&&", "||", "(", ")", "==", "!=", "<=", ">=", "<", ">", "!"};
    const std::unordered_set<std::string> comp_operands = {"==", "!=", "<=", ">=", "<", ">"};

    std::unordered_map<std::string, std::string> ffun_sufix = {{"liczba", "d"}, {"logika", "hhd"}, {"adres", "lld"}};

    std::unordered_map<std::string, std::string> operand_type = {
        {"+", "liczba"},
        {"-", "liczba"},
        {"*", "liczba"},
        {"/", "liczba"},
        {"%", "liczba"},
        {"<<", "liczba"},
        {">>", "liczba"},
        {"&&", "logika"},
        {"||", "logika"},
        {"!", "logika"},
        {"==", "liczba"},
        {"!=", "liczba"},
        {"<=", "liczba"},
        {">=", "liczba"},
        {"<", "liczba"},
        {">", "liczba"}};

    std::unordered_map<std::string, int> operand_priority = {
        {"||", 1},
        {"&&", 2},
        {"==", 3},
        {"!=", 3},
        {"<", 4},
        {">", 4},
        {"<=", 4},
        {">=", 4},
        {"<<", 5},
        {">>", 5},
        {"+", 6},
        {"-", 6},
        {"*", 7},
        {"/", 7},
        {"%", 7},
        {"!", 8}};

    std::unordered_map<std::string, std::string> negation =
        {
            {"==", "!="},
            {"!=", "=="},

            {"<", ">="},
            {">=", "<"},

            {">", "<="},
            {"<=", ">"},

            {"&&", "||"},
            {"||", "&&"}};

    std::unordered_map<std::string, std::unordered_map<int, std::string>> type_num_to_reg = {
        {"liczba",
         {{0, "ecx"},
          {1, "edx"},
          {2, "r8d"},
          {3, "r9d"}}},
        {"logika",
         {{0, "cl"},
          {1, "dl"},
          {2, "r8b"},
          {3, "r9b"}}},
        {"adres",
         {{0, "rcx"},
          {1, "rdx"},
          {2, "r8"},
          {3, "r9"}}}};

    const std::unordered_map<std::string, std::string> type_to_return_reg = {
        {"logika", "al"},
        {"liczba", "eax"},
        {"adres", "rax"}};

    const std::unordered_map<std::string, int> type_rank = {
        {"logika", 1},
        {"liczba", 2},
        {"adres", 3}};

    void remember_variable_type(const std::string &name, const std::string &type)
    {
        var_types[name] = type;
    }

    std::string promoted_type(const std::string &left, const std::string &right)
    {
        int left_rank = type_rank.count(left) ? type_rank.at(left) : 0;
        int right_rank = type_rank.count(right) ? type_rank.at(right) : 0;

        if (left_rank >= right_rank)
            return left;
        return right;
    }

    std::string register_type(const std::string &s)
    {
        if (s == "al" || s == "cl" || s == "dl" || s == "r8b" || s == "r9b")
            return "logika";
        if (s == "eax" || s == "ecx" || s == "edx" || s == "r8d" || s == "r9d")
            return "liczba";
        if (s == "rax" || s == "rcx" || s == "rdx" || s == "r8" || s == "r9")
            return "adres";
        return {};
    }

    bool wrapped_in_outer_parentheses(const std::string &s)
    {
        if (s.size() < 2 || s.front() != '(' || s.back() != ')')
            return false;

        int depth = 0;
        for (std::size_t i = 0; i < s.size(); i++)
        {
            if (s[i] == '(')
                depth++;
            else if (s[i] == ')')
                depth--;

            if (depth == 0 && i + 1 < s.size())
                return false;
        }

        return depth == 0;
    }

    std::string get_var_name(const std::string s)
    {
        
        int id = 0;
        while (var_names.count(s + '_' + std::to_string(id)))
            id++;
        var_names.insert(s + '_' + std::to_string(id));
        return s + '_' + std::to_string(id);
    }

    std::string get_stop_name(const std::string s)
    {
        int id = 0;
        while (stop_names.count(s + '_' + std::to_string(id)))
            id++;
        stop_names.insert(s + '_' + std::to_string(id));
        return s + '_' + std::to_string(id);
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

    int count_opperands(const std::string &s)
    {
        int i = 0, res = 0;
        while (i < s.size())
        {
            if (i + 1 < s.size())
            {
                std::string temp_1 = s.substr(i, 2), temp_2 = s.substr(i, 1);
                if (operands.count(temp_1))
                {
                    i++;
                    res++;
                }
                else if (operands.count(temp_2))
                    res++;
            }
            else if (s.back() == ')')
                res++;
            i++;
        }
        return res;
    }

    void split(const std::string &s, std::string &beg, std::string &sign, std::string &end)
    {
        for (std::size_t i = 0; i < s.size(); i++)
        {
            std::string one = s.substr(i, 1);
            std::string two;

            if (i + 1 < s.size())
                two = s.substr(i, 2);

            if (operands.count(two))
            {
                beg = s.substr(0, i);
                sign = two;
                end = s.substr(i + 2);
                break;
            }
            else if (operands.count(one))
            {
                beg = s.substr(0, i);
                sign = one;
                end = s.substr(i + 1);
                break;
            }
        }
    }

    std::string remove_spaces(std::string s)
    {
        for (std::size_t i = 0; i < s.size(); i++)
            while (i < s.size() && s[i] == ' ')
                s.erase(i, 1);
        return s;
    }

    bool find_top_level_operator(const std::string &s, std::size_t &idn, std::string &op)
    {
        int lay = 0, best_priority = 100;
        idn = std::string::npos;
        op.clear();

        for (std::size_t i = 0; i < s.size(); i++)
        {
            if (s[i] == '(')
                lay++;
            else if (s[i] == ')')
                lay--;

            if (lay != 0)
                continue;

            std::string cur_op;
            if (i + 1 < s.size() && operands.count(s.substr(i, 2)))
                cur_op = s.substr(i, 2);
            else if (operands.count(s.substr(i, 1)))
                cur_op = s.substr(i, 1);

            if (!cur_op.empty() && operand_priority.count(cur_op) && operand_priority[cur_op] < best_priority)
            {
                best_priority = operand_priority[cur_op];
                idn = i;
                op = cur_op;
            }
        }

        return idn != std::string::npos;
    }

    std::string expression_value_type(const std::string &expr, const std::string &fallback)
    {
        std::string s = remove_spaces(expr);
        while (wrapped_in_outer_parentheses(s))
            s = s.substr(1, s.size() - 2);

        if (s.empty() || is_num(s))
            return fallback;

        if (var_types.count(s))
            return var_types[s];

        std::string reg_type = register_type(s);
        if (!reg_type.empty())
            return reg_type;

        std::size_t idn = 0;
        std::string sign;
        if (!find_top_level_operator(s, idn, sign))
            return fallback;

        if (sign == "!" || sign == "&&" || sign == "||" ||
            sign == "==" || sign == "!=" || sign == "<=" || sign == ">=" || sign == "<" || sign == ">")
            return "logika";

        std::string beg = s.substr(0, idn);
        std::string end = s.substr(idn + sign.size());
        return promoted_type(expression_value_type(beg, fallback), expression_value_type(end, fallback));
    }

    std::string operation_operand_type(const std::string &sign, const std::string &beg, const std::string &end, const std::string &fallback)
    {
        if (sign == "!" || sign == "&&" || sign == "||")
            return "logika";

        return promoted_type(expression_value_type(beg, fallback), expression_value_type(end, fallback));
    }

    bool is_known_value(const std::string &s, const std::unordered_map<std::string, long long> &know, long long &value)
    {
        if (is_num(s))
        {
            value = std::stoll(s);
            return true;
        }

        auto it = know.find(s);
        if (it == know.end())
            return false;

        value = it->second;
        return true;
    }

    bool parse_binary(const std::string &expr, std::string &beg, std::string &sign, std::string &end)
    {
        split(expr, beg, sign, end);
        beg = remove_spaces(beg);
        end = remove_spaces(end);
        return !sign.empty();
    }

    void add_expr_variables(const std::string &expr, std::vector<std::string> &vars)
    {
        std::string token;
        auto flush = [&]()
        {
            if (!token.empty())
            {
                if (!is_num(token) && var_names.count(token))
                    vars.push_back(token);
                token.clear();
            }
        };

        for (char c : expr)
        {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
                token.push_back(c);
            else
                flush();
        }
        flush();
    }

    std::vector<std::string> used_variables(const std::string &name, const std::vector<std::string> &arg)
    {
        std::vector<std::string> vars;

        if (name == "ustaw" && arg.size() >= 2)
            add_expr_variables(arg[1], vars);
        else if ((name == "idzjezeli" || name == "zakoncz") && !arg.empty())
            add_expr_variables(arg[0], vars);
        else if (name == "wczytaj_ram" && arg.size() >= 3)
        {
            add_expr_variables(arg[1], vars);
            add_expr_variables(arg[2], vars);
        }
        else if (name == "zapisz" && arg.size() >= 3)
        {
            add_expr_variables(arg[0], vars);
            add_expr_variables(arg[1], vars);
            add_expr_variables(arg[2], vars);
        }
        else if (name == "wypisz")
        {
            for (std::size_t i = 1; i < arg.size(); i++)
                add_expr_variables(arg[i], vars);
        }

        return vars;
    }

    std::vector<std::string> edited_variables(const std::string &name, const std::vector<std::string> &arg)
    {
        std::vector<std::string> vars;

        if (name == "ustaw" && !arg.empty())
            add_expr_variables(arg[0], vars);
        else if (name == "wczytaj_ram" && !arg.empty())
            add_expr_variables(arg[0], vars);
        else if (name == "wczytaj")
        {
            for (std::size_t i = 1; i < arg.size(); i++)
                add_expr_variables(arg[i], vars);
        }

        return vars;
    }

    std::string first_variable(const std::string &expr)
    {
        std::string token;
        for (char c : expr)
        {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
                token.push_back(c);
            else if (!token.empty())
                break;
        }

        if (!token.empty() && var_names.count(token))
            return token;
        return {};
    }

    void add_variable_counts(const std::vector<std::string> &vars, std::unordered_map<std::string, int> &total_usage)
    {
        for (const std::string &name : vars)
            total_usage[name]++;
    }

    bool eval_operation(const std::string &sign, long long val_1, long long val_2, long long &res)
    {
        if (sign == "+")
            res = val_1 + val_2;
        else if (sign == "-")
            res = val_1 - val_2;
        else if (sign == "*")
            res = val_1 * val_2;
        else if (sign == "/")
        {
            if (val_2 == 0)
                throw std::runtime_error("dzielenie przez 0");
            res = val_1 / val_2;
        }
        else if (sign == "%")
        {
            if (val_2 == 0)
                throw std::runtime_error("dzielenie przez 0");
            res = val_1 % val_2;
        }
        else if (sign == "<<")
            res = val_1 << val_2;
        else if (sign == ">>")
            res = val_1 >> val_2;
        else if (sign == "&&")
            res = val_1 && val_2;
        else if (sign == "||")
            res = val_1 || val_2;
        else if (sign == "==")
            res = val_1 == val_2;
        else if (sign == "!=")
            res = val_1 != val_2;
        else if (sign == "<=")
            res = val_1 <= val_2;
        else if (sign == ">=")
            res = val_1 >= val_2;
        else if (sign == "<")
            res = val_1 < val_2;
        else if (sign == ">")
            res = val_1 > val_2;
        else if (sign == "!")
            res = !val_2;
        else
            return false;

        return true;
    }

    bool simplify_identity(std::string &expr)
    {
        expr = remove_spaces(expr);

        std::string beg, sign, end;
        if (!parse_binary(expr, beg, sign, end))
            return false;

        std::string simplified = expr;

        if (sign == "!")
        {
            if (!end.empty() && end[0] == '!')
                simplified = end.substr(1);
        }
        else if (sign == "||")
        {
            if (beg == "1" || end == "1")
                simplified = "1";
            else if (beg == "0")
                simplified = end;
            else if (end == "0")
                simplified = beg;
            else if (beg == end)
                simplified = beg;
        }
        else if (sign == "&&")
        {
            if (beg == "0" || end == "0")
                simplified = "0";
            else if (beg == "1")
                simplified = end;
            else if (end == "1")
                simplified = beg;
            else if (beg == end)
                simplified = beg;
        }
        else if (sign == "+")
        {
            if (beg == "0")
                simplified = end;
            else if (end == "0")
                simplified = beg;
        }
        else if (sign == "-")
        {
            if (end == "0")
                simplified = beg;
            else if (beg == end)
                simplified = "0";
        }
        else if (sign == "*")
        {
            if (beg == "0" || end == "0")
                simplified = "0";
            else if (beg == "1")
                simplified = end;
            else if (end == "1")
                simplified = beg;
        }
        else if (sign == "/")
        {
            if (end == "1")
                simplified = beg;
        }
        else if (sign == "%")
        {
            if (end == "1")
                simplified = "0";
        }
        else if (sign == "<<" || sign == ">>")
        {
            if (end == "0")
                simplified = beg;
        }
        else if (beg == end)
        {
            if (sign == "==" || sign == "<=" || sign == ">=")
                simplified = "1";
            else if (sign == "!=" || sign == "<" || sign == ">")
                simplified = "0";
        }

        if (simplified == expr)
            return false;

        expr = simplified;
        return true;
    }

    struct ArtDelta
    {
        std::string base;
        long long delta = 0;
    };

    struct ArrayRef
    {
        std::string name;
        std::string index;
        std::size_t begin = 0;
        std::size_t end = 0;
    };

    bool is_ident_char(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    }

    bool parse_array_ref_at(const std::string &expr, std::size_t pos, const std::unordered_map<std::string, std::string> &array_types, ArrayRef &ref)
    {
        if (pos >= expr.size() || !is_ident_char(expr[pos]))
            return false;

        std::size_t name_begin = pos;
        while (pos < expr.size() && is_ident_char(expr[pos]))
            pos++;

        std::string name = expr.substr(name_begin, pos - name_begin);
        if (!array_types.count(name) || pos >= expr.size() || expr[pos] != '[')
            return false;

        std::size_t index_begin = pos + 1;
        int depth = 1;
        pos++;
        while (pos < expr.size() && depth > 0)
        {
            if (expr[pos] == '[')
                depth++;
            else if (expr[pos] == ']')
                depth--;

            if (depth > 0)
                pos++;
        }

        if (depth != 0)
            return false;

        std::string index = remove_spaces(expr.substr(index_begin, pos - index_begin));
        if (index.empty())
            return false;

        ref.name = name;
        ref.index = index;
        ref.begin = name_begin;
        ref.end = pos + 1;
        return true;
    }

    bool find_array_ref(const std::string &expr, const std::unordered_map<std::string, std::string> &array_types, ArrayRef &ref)
    {
        for (std::size_t i = 0; i < expr.size(); i++)
            if (parse_array_ref_at(expr, i, array_types, ref))
                return true;
        return false;
    }

    bool parse_full_array_ref(const std::string &expr, const std::unordered_map<std::string, std::string> &array_types, ArrayRef &ref)
    {
        std::string normalized = remove_spaces(expr);
        return parse_array_ref_at(normalized, 0, array_types, ref) && ref.begin == 0 && ref.end == normalized.size();
    }

    bool parse_assignment_art(const std::string &target, const std::string &expr, ArtDelta &art)
    {
        std::string normalized = remove_spaces(expr);
        if (normalized == target)
        {
            art.base = target;
            art.delta = 0;
            return true;
        }

        std::string beg, sign, end;
        if (!parse_binary(normalized, beg, sign, end))
            return false;

        if (sign == "+" && beg == target && is_num(end))
        {
            art.base = target;
            art.delta = std::stoll(end);
            return true;
        }

        if (sign == "+" && end == target && is_num(beg))
        {
            art.base = target;
            art.delta = std::stoll(beg);
            return true;
        }

        if (sign == "-" && beg == target && is_num(end))
        {
            art.base = target;
            art.delta = -std::stoll(end);
            return true;
        }

        return false;
    }

    std::string array_value_key(const std::string &array_name, const std::string &index)
    {
        return array_name + "[" + index + "]";
    }

    void clear_array_values(std::unordered_map<std::string, long long> &array_know, const std::string &array_name)
    {
        const std::string prefix = array_name + "[";
        for (auto it = array_know.begin(); it != array_know.end();)
        {
            if (it->first.rfind(prefix, 0) == 0)
                it = array_know.erase(it);
            else
                it++;
        }
    }

    std::string art_to_expr(const ArtDelta &art)
    {
        if (art.delta == 0)
            return art.base;
        if (art.delta > 0)
            return art.base + "+" + std::to_string(art.delta);
        return art.base + "-" + std::to_string(-art.delta);
    }

    bool predict_iterations(long long start, const std::string &sign, long long limit, long long step, int &iterations)
    {
        if (step == 0)
            return false;

        long long current = start;
        int count = 0;
        constexpr int max_unroll = 32;

        while (true)
        {
            bool condition = false;
            if (sign == ">")
                condition = current > limit;
            else if (sign == ">=")
                condition = current >= limit;
            else if (sign == "<")
                condition = current < limit;
            else if (sign == "<=")
                condition = current <= limit;
            else if (sign == "!=")
                condition = current != limit;
            else if (sign == "==")
                condition = current == limit;
            else
                return false;

            if (!condition)
                break;

            if (count == max_unroll)
                return false;

            if ((step > 0 && current > std::numeric_limits<long long>::max() - step) ||
                (step < 0 && current < std::numeric_limits<long long>::min() - step))
                return false;

            current += step;
            count++;
        }

        iterations = count;
        return true;
    }

    std::vector<std::string> split_string(const std::string &s)
    {
        std::string buf = "";
        std::vector<std::string> res;
        int depth = 0;
        for (char c : s)
        {
            if (c == '(')
                depth++;
            else if (c == ')')
                depth--;

            if (c == ',' && depth == 0)
            {
                if (!buf.empty())
                    res.push_back(remove_spaces(buf));
                buf = "";
            }
            else
                buf += c;
        }

        if (!buf.empty())
            res.push_back(remove_spaces(buf));
        return res;
    }

    bool parse_function_call(const std::string &expr, const std::unordered_map<std::string, std::string> &fun_type, std::string &name, std::vector<std::string> &args)
    {
        std::string s = remove_spaces(expr);
        std::size_t open = s.find('(');
        if (open == std::string::npos || open == 0 || s.empty() || s.back() != ')')
            return false;

        name = s.substr(0, open);
        if (!fun_type.count(name))
            return false;

        int depth = 0;
        for (std::size_t i = open; i < s.size(); i++)
        {
            if (s[i] == '(')
                depth++;
            else if (s[i] == ')')
                depth--;

            if (depth == 0 && i + 1 < s.size())
                return false;
            if (depth < 0)
                return false;
        }
        if (depth != 0)
            return false;

        std::string raw_args = s.substr(open + 1, s.size() - open - 2);
        args = split_string(raw_args);
        return true;
    }

    bool contains_function_call(const std::string &expr, const std::unordered_map<std::string, std::string> &fun_type)
    {
        std::string s = remove_spaces(expr);
        for (std::size_t i = 0; i < s.size(); i++)
        {
            if (!is_ident_char(s[i]))
                continue;

            std::size_t begin = i;
            while (i < s.size() && is_ident_char(s[i]))
                i++;

            std::string name = s.substr(begin, i - begin);
            if (fun_type.count(name) && i < s.size() && s[i] == '(')
                return true;
        }

        return false;
    }
}

Tree::Node::Node(std::string name, Node *parent) : name(name), parent(parent) {}
void Tree::Node::print(int ofs) const
{
    std::cout << std::string(ofs, ' ') << name << "[";
    for (std::size_t i = 0; i < arg.size(); i++)
    {
        std::cout << arg[i];
        if (i + 1 != arg.size())
            std::cout << ',';
    }
    std::cout << "]";
    if (!child.empty())
        std::cout << ':';
    std::cout << "\n";
    for (const auto &n : child)
        n->print(4 + ofs);
}

std::vector<Token> Tree::Node::get_tokens(std::vector<Token> &tok) const
{
    Token cur;
    cur.name = name;
    for (std::size_t i = 0; i < arg.size(); i++)
        cur.arg.push_back(arg[i]);

    tok.push_back(cur);

    for (const auto &n : child)
        n->get_tokens(tok);

    return tok;
}

Tree::Node::~Node()
{
    for (Node *c : child)
        delete c;
}

Tree::Tree() { cur = beg = new Node("enter_point", nullptr); }

void Tree::append(const Token &tok)
{
    if (tok.name == "Pusty")
        return;

    if (tok.name == "begin")
    {
        if (cur->name == "enter_point" && cur->child.empty())
            return;
        else if (cur->child.empty())
            std::__throw_logic_error("Nie ma do czego wejść");
        Node *next = cur->child.back();
        cur = next;
        return;
    }

    if (tok.name == "end")
    {
        if (cur->name == "enter_point")
            return;
        else if (cur->parent == nullptr)
            std::__throw_logic_error("Nie ma z czego wyjść");
        Node *next = cur->parent;
        cur = next;
        return;
    }

    if (tok.name == "stworz")
    {
        var_names.insert(tok.arg.back());
        if (tok.arg[0] == "tablica" && tok.arg.size() >= 4)
            remember_variable_type(tok.arg.back(), "tablica_" + tok.arg[1]);
        else
            remember_variable_type(tok.arg.back(), tok.arg[0]);
    }

    Node *line = new Node(tok.name, cur);
    cur->child.push_back(line);
    line->arg = move(tok.arg);
}

int Tree::node_traversal(Node *cur)
{
    if (cur->name == "enter_point")
    {
        std::vector<std::string> new_args;
        std::vector<Node *> prefix;

        new_args.push_back(cur->arg[0]);
        new_args.push_back(cur->arg[1]);

        for (int i = 2; i + 1 < cur->arg.size(); i += 2)
        {
            int arg_num = i / 2 - 1;
            const std::string &type = cur->arg[i];
            const std::string &name = cur->arg[i + 1];
            if (!type_num_to_reg.count(type) || !type_num_to_reg[type].count(arg_num))
                throw std::runtime_error("Nieobslugiwany argument funkcji: " + name);

            Node *cre = new Node("stworz", cur);
            cre->arg.push_back(type);
            cre->arg.push_back(name);
            remember_variable_type(name, type);

            Node *set = new Node("ustaw", cur);
            set->arg.push_back(name);

            var_names.insert(name);

            std::string reg = type_num_to_reg[type][arg_num];
            set->arg.push_back(reg);

            prefix.push_back(cre);
            prefix.push_back(set);
        }

        cur->child.insert(cur->child.begin(), prefix.begin(), prefix.end());
        cur->arg = new_args;
    }

    if (cur->name == "dopoki")
    {
        Node *par = cur->parent;

        std::size_t idx = 0;
        while (par->child[idx] != cur)
            idx++;

        std::string name = get_stop_name(beg->arg[1] + "_loop");
        auto &v = par->child;

        Node *go_to = new Node("idz", cur);
        cur->child.push_back(go_to);

        Node *point = new Node("punkt", par);
        v.insert(v.begin() + idx, point);

        cur->name = "jezeli";
        go_to->arg.push_back(name);
        point->arg.push_back(name);

        return 0;
    }

    if (cur->name == "albo")
    {
        Node *par = cur->parent;
        auto &v = par->child;

        auto it = std::find(v.begin(), v.end(), cur);
        if (it == v.end())
            return 0;

        std::size_t idx = std::distance(v.begin(), it);

        v.erase(it);

        for (std::size_t i = 0; i < cur->child.size(); ++i)
        {
            Node *child = cur->child[i];
            child->parent = par;
            v.insert(v.begin() + idx + i, child);
        }
        cur->child.clear();

        delete cur;
        return 1;
    }

    if (cur->name == "albojezeli")
    {
        Node *par = cur->parent;
        auto &v = par->child;

        std::size_t idx = 0;
        while (par->child[idx] != cur)
            idx++;

        std::size_t block_size = 1;
        while (idx + block_size < v.size() && (v[idx + block_size]->name == "albojezeli" || v[idx + block_size]->name == "albo"))
            block_size++;

        if (v[idx + block_size]->name != "punkt")
            throw std::runtime_error("Blok jezeli jest popsuty (wina klompliatora) :(");

        std::string punkt_label = v[idx + block_size]->arg[0];
        Node *idz = new Node("idz", cur);
        idz->arg.push_back(punkt_label);
        cur->child.push_back(idz);

        cur->name = "jezeli";
    }

    if (cur->name == "jezeli" && !var_names.count(cur->arg[0]))
    {
        Node *par = cur->parent;

        std::size_t idx = 0;
        auto &v = par->child;
        while (v[idx] != cur)
            idx++;

        std::size_t block_size = 1;
        while (idx + block_size < v.size() && (v[idx + block_size]->name == "albojezeli" || v[idx + block_size]->name == "albo"))
            block_size++;

        if (block_size > 1)
        {
            if (!(idx + block_size < v.size() && v[idx + block_size]->name == "punkt" && !cur->child.empty() && cur->child.back()->name == "idz" && cur->child.back()->arg[0] == v[idx + block_size]->arg[0]))
            {
                std::string punkt_label = get_stop_name(beg->arg[1]+ "_blockend");
                Node *punkt = new Node("punkt", par);
                punkt->arg.push_back(punkt_label);
                v.insert(v.begin() + idx + block_size, punkt);

                Node *idz = new Node("idz", cur);
                idz->arg.push_back(punkt_label);
                cur->child.push_back(idz);
            }
        }

        std::string name = get_var_name("logika");

        Node *set = new Node("ustaw", par);
        v.insert(v.begin() + idx, set);

        Node *creator = new Node("stworz", par);
        v.insert(v.begin() + idx, creator);

        creator->arg.push_back("logika");
        creator->arg.push_back(name);
        remember_variable_type(name, "logika");

        set->arg.push_back(name);
        set->arg.push_back(cur->arg[0]);

        cur->arg[0] = name;

        return 0;
    }

    if (cur->name == "jezeli" && var_names.count(cur->arg[0]))
    {
        Node *par = cur->parent;

        std::size_t idx = 0;
        while (par->child[idx] != cur)
            idx++;

        auto &v = par->child;

        Node *neg = new Node("ustaw", par);
        v.insert(v.begin() + idx, neg);

        neg->arg.push_back(cur->arg[0]);
        neg->arg.push_back("!" + cur->arg[0]);

        std::string stop = get_stop_name(beg->arg[1] + "_endif");
        Node *end = new Node("punkt", cur);
        cur->child.push_back(end);
        end->arg.push_back(stop);

        cur->arg.push_back(stop);
        cur->name = "idzjezeli";

        return 0;
    }

    if (cur->name == "wczytaj" && cur->arg.size() == 1)
    {
        std::vector<std::string> vars;
        add_expr_variables(cur->arg[0], vars);
        if (vars.empty())
            throw std::runtime_error("Brak zmiennych w wczytaj");

        cur->arg[0] = "\"";
        for (const std::string &name : vars)
            cur->arg[0] += "%" + ffun_sufix[var_types[name]];
        cur->arg[0] += "\"";

        for (const auto &s : vars)
            cur->arg.push_back(s);
    }

    if (cur->name == "wypisz" && cur->arg.size() == 1)
    {
        std::vector<std::string> vars;
        std::string buf = "", &arg = cur->arg[0];
        bool read = false;
        std::size_t start = 0;

        for (std::size_t i = 0; i < arg.size(); i++)
        {
            char c = arg[i];

            if (c == '%')
            {
                read = true;
                buf = "";
                start = i + 1;
                continue;
            }

            if (read)
                buf += c;

            if (read && var_names.count(buf))
            {
                vars.push_back(buf);

                std::string suf = ffun_sufix[var_types[buf]];
                arg.replace(start, buf.size(), suf);
                i = start + suf.size() - 1;

                buf = "";
                read = false;
            }

            if (c == ' ')
            {
                buf = "";
                read = false;
            }
        }

        for (const auto s : vars)
            cur->arg.push_back(s);
    }

    if (cur->name == "zakoncz" && (!var_names.count(cur->arg[0]) && !is_num(cur->arg[0])))
    {
        Node *par = cur->parent;
        Node *god = par;
        while (god->parent != nullptr)
            god = god->parent;

        std::size_t idx = 0;
        auto &v = par->child;
        while (v[idx] != cur)
            idx++;

        std::string temp_name = get_var_name("zakoncz");
        Node *cre = new Node("stworz", par);

        cre->arg.push_back(god->arg[0]);
        cre->arg.push_back(temp_name);
        remember_variable_type(temp_name, god->arg[0]);

        Node *set = new Node("ustaw", par);
        set->arg.push_back(temp_name);
        set->arg.push_back(cur->arg[0]);

        cur->arg[0] = temp_name;

        v.insert(v.begin() + idx, set);
        v.insert(v.begin() + idx, cre);
    }

    if (cur->name == "ustaw")
    {
        Node *par = cur->parent;

        std::size_t idx = 0;
        while (par->child[idx] != cur)
            idx++;

        auto &v = par->child;

        std::string &s = cur->arg[1];

        for (int i = 0; i < s.size(); i++)
            while (i < s.size() && s[i] == ' ')
                s.erase(i, 1);

        std::string fun_name;
        std::vector<std::string> call_args;
        if (parse_function_call(s, fun_type, fun_name, call_args))
        {
            std::vector<std::string> arg_types;
            auto fun_it = fun_args.find(fun_name);
            if (fun_it != fun_args.end())
                arg_types = fun_it->second;

            if (call_args.size() != arg_types.size())
                throw std::runtime_error("Zla liczba argumentow wywolania funkcji: " + fun_name);

            std::vector<Node *> prefix;
            bool has_nested_call = false;
            for (const std::string &arg : call_args)
                has_nested_call |= contains_function_call(arg, fun_type);

            auto append_arg_register_load = [&](std::size_t i, const std::string &expr)
            {
                const std::string &type = arg_types[i];
                if (!type_num_to_reg.count(type) || !type_num_to_reg[type].count(static_cast<int>(i)))
                    throw std::runtime_error("Nieobslugiwany argument wywolania funkcji: " + fun_name);

                std::string reg = type_num_to_reg[type][static_cast<int>(i)];
                Node *set_reg = new Node("ustaw", par);
                set_reg->arg.push_back(reg);
                set_reg->arg.push_back(expr);
                prefix.push_back(set_reg);
            };

            if (has_nested_call)
            {
                std::vector<std::string> temp_names;
                for (std::size_t i = 0; i < call_args.size(); i++)
                {
                    const std::string &type = arg_types[i];
                    if (!type_num_to_reg.count(type) || !type_num_to_reg[type].count(static_cast<int>(i)))
                        throw std::runtime_error("Nieobslugiwany argument wywolania funkcji: " + fun_name);

                    std::string temp_name = get_var_name(type);
                    remember_variable_type(temp_name, type);
                    temp_names.push_back(temp_name);

                    Node *create = new Node("stworz", par);
                    create->arg.push_back(type);
                    create->arg.push_back(temp_name);
                    prefix.push_back(create);

                    Node *set_temp = new Node("ustaw", par);
                    set_temp->arg.push_back(temp_name);
                    set_temp->arg.push_back(call_args[i]);
                    prefix.push_back(set_temp);
                }

                for (std::size_t i = 0; i < temp_names.size(); i++)
                    append_arg_register_load(i, temp_names[i]);
            }
            else
            {
                for (std::size_t i = 0; i < call_args.size(); i++)
                    append_arg_register_load(i, call_args[i]);
            }

            Node *call = new Node("wywolaj", par);
            call->arg.push_back(fun_name);
            prefix.push_back(call);

            v.insert(v.begin() + idx, prefix.begin(), prefix.end());

            cur->arg[1] = type_to_return_reg.at(fun_type[fun_name]);
            return 0;
        }

        if (count_opperands(s) > 1)
        {
            int idn = -1, lay = 0, best_priority = 100;
            for (int i = 0; i + 1 < s.size(); i++)
            {
                if (s[i] == '(')
                    lay++;
                else if (s[i] == ')')
                    lay--;

                if (lay == 0)
                {
                    std::string op;

                    if (i + 1 < static_cast<int>(s.size()) && operands.count(s.substr(i, 2)))
                        op = s.substr(i, 2);
                    else if (operands.count(s.substr(i, 1)))
                        op = s.substr(i, 1);

                    if (!op.empty() && operand_priority.count(op))
                    {
                        if (operand_priority[op] < best_priority)
                        {
                            best_priority = operand_priority[op];
                            idn = i;
                        }
                    }
                }
            }

            if (idn == -1)
            {
                s.pop_back();
                s.erase(0, 1);
            }
            else
            {
                std::string beg = s.substr(0, idn), sign, end, two = s.substr(idn, 2);

                if (operands.count(two))
                {
                    sign = two;
                    end = s.substr(idn + 2);
                }
                else
                {
                    sign = s.substr(idn, 1);
                    end = s.substr(idn + 1);
                }

                if (sign == "!")
                {
                    Node *edit = new Node("ustaw", par);
                    edit->arg.push_back(cur->arg[0]);
                    edit->arg.push_back("!" + cur->arg[0]);
                    v.insert(v.begin() + idx + 1, edit);
                    cur->arg[1] = end;
                }
                else
                {
                    std::string fallback = expression_value_type(cur->arg[0], "liczba");
                    std::string type = operation_operand_type(sign, beg, end, fallback);
                    std::string temp_name_1 = get_var_name(type), temp_name_2 = get_var_name(type);
                    remember_variable_type(temp_name_1, type);
                    remember_variable_type(temp_name_2, type);

                    Node *set_1 = new Node("ustaw", par);
                    set_1->arg.push_back(temp_name_1);
                    set_1->arg.push_back(beg);
                    v.insert(v.begin() + idx, set_1);

                    Node *create_1 = new Node("stworz", par);
                    create_1->arg.push_back(type);
                    create_1->arg.push_back(temp_name_1);
                    v.insert(v.begin() + idx, create_1);

                    Node *set_2 = new Node("ustaw", par);
                    set_2->arg.push_back(temp_name_2);
                    set_2->arg.push_back(end);
                    v.insert(v.begin() + idx, set_2);

                    Node *create_2 = new Node("stworz", par);
                    create_2->arg.push_back(type);
                    create_2->arg.push_back(temp_name_2);
                    v.insert(v.begin() + idx, create_2);

                    cur->arg[1] = temp_name_1 + sign + temp_name_2;
                }
            }

            return 0;
        }
    }

    for (std::size_t i = 0; i < cur->child.size(); i += node_traversal(cur->child[i]))
        ;
    return 1;
}

void Tree::traversal()
{
    node_traversal(beg);
}

bool Tree::constant_folding_node(Node *cur, std::unordered_map<std::string, long long> &know, std::unordered_map<std::string, long long> &array_know)
{
    bool changed = 0;

    auto fold_known_arg = [&](std::string &arg)
    {
        arg = remove_spaces(arg);

        long long value = 0;
        if (!is_known_value(arg, know, value))
            return false;

        std::string folded = std::to_string(value);
        bool local_changed = arg != folded;
        arg = folded;
        return local_changed;
    };

    if (cur->name == "idzjezeli" || cur->name == "zakoncz")
        if (know.count(cur->arg[0]))
            cur->arg[0] = std::to_string(know[cur->arg[0]]);

    if (cur->name == "punkt" || cur->name == "idzjezeli" || cur->name == "idz" || cur->name == "wywolaj")
    {
        know.clear();
        array_know.clear();
    }

    if (cur->name == "ustaw")
    {
        cur->arg[1] = remove_spaces(cur->arg[1]);

        if (count_opperands(cur->arg[1]) == 0)
        {
            if (is_num(cur->arg[1]))
                know[cur->arg[0]] = std::stoll(cur->arg[1]);
            else if (know.count(cur->arg[1]))
                know[cur->arg[0]] = know[cur->arg[1]];
            else
                know.erase(cur->arg[0]);

            if (cur->arg[0] == cur->arg[1])
            {
                Node *par = cur->parent;
                std::size_t idx = 0;
                while (idx < par->child.size() && par->child[idx] != cur)
                    idx++;

                par->child.erase(par->child.begin() + idx);
            }
        }
        else
        {
            std::string s = cur->arg[1], beg, sign, end;
            parse_binary(s, beg, sign, end);

            long long val_1 = 0, val_2 = 0;
            bool has_1 = is_known_value(beg, know, val_1);
            bool has_2 = is_known_value(end, know, val_2);

            long long res;
            if (((has_1 && has_2) || (sign == "!" && has_2)) && eval_operation(sign, val_1, val_2, res))
            {
                know[cur->arg[0]] = res;
                cur->arg[1] = std::to_string(res);
                changed = 1;
            }
            else
                know.erase(cur->arg[0]);
        }
    }

    if (cur->name == "wczytaj_ram" && cur->arg.size() >= 3)
    {
        changed |= fold_known_arg(cur->arg[2]);

        const std::string key = array_value_key(cur->arg[1], cur->arg[2]);
        auto it = array_know.find(key);
        if (is_num(cur->arg[2]) && it != array_know.end())
        {
            cur->name = "ustaw";
            cur->arg = {cur->arg[0], std::to_string(it->second)};
            know[cur->arg[0]] = it->second;
            changed = true;
        }
        else
            know.erase(cur->arg[0]);
    }
    else if (cur->name == "wczytaj")
    {
        for (const std::string &name : edited_variables(cur->name, cur->arg))
            know.erase(name);
    }
    else if (cur->name == "wypisz")
    {
        for (std::size_t i = 1; i < cur->arg.size(); i++)
            changed |= fold_known_arg(cur->arg[i]);
    }
    else if (cur->name == "zapisz" && cur->arg.size() >= 3)
    {
        changed |= fold_known_arg(cur->arg[0]);
        changed |= fold_known_arg(cur->arg[2]);

        if (is_num(cur->arg[2]))
        {
            long long value = 0;
            const std::string key = array_value_key(cur->arg[1], cur->arg[2]);
            if (is_known_value(cur->arg[0], know, value))
                array_know[key] = value;
            else
                array_know.erase(key);
        }
        else
            clear_array_values(array_know, cur->arg[1]);
    }

    for (std::size_t i = 0; i < cur->child.size(); i++)
        changed |= constant_folding_node(cur->child[i], know, array_know);

    return changed;
}

bool Tree::constant_folding()
{
    std::unordered_map<std::string, long long> know;
    std::unordered_map<std::string, long long> array_know;
    return constant_folding_node(beg, know, array_know);
}

bool Tree::branch_cutting_node(Node *cur)
{
    bool changed = false;

    for (std::size_t i = 0; i < cur->child.size(); i++)
        changed |= branch_cutting_node(cur->child[i]);

    if (cur->name == "idzjezeli")
    {
        Node *par = cur->parent;

        if (par == nullptr)
            return changed;

        std::size_t idx = 0;
        while (idx < par->child.size() && par->child[idx] != cur)
            idx++;

        if (idx == par->child.size())
            return changed;

        auto &v = par->child;

        if (cur->arg[0] == "1")
        {
            v.erase(v.begin() + idx);
            delete cur;

            changed = true;
        }
        else if (cur->arg[0] == "0")
        {
            std::size_t children_to_move = 0;

            if (!cur->child.empty())
                children_to_move = cur->child.size() - 1;

            for (std::size_t i = 0; i < children_to_move; i++)
            {
                Node *child = cur->child[i];
                child->parent = par;
                v.insert(v.begin() + idx + i, child);
            }

            v.erase(v.begin() + idx + children_to_move);

            cur->child.erase(cur->child.begin(), cur->child.begin() + children_to_move);

            delete cur;

            changed = true;
        }
    }

    return changed;
}

bool Tree::branch_cutting()
{
    return branch_cutting_node(beg);
}

bool Tree::identity_simplification_node(Node *cur)
{
    bool changed = false;

    if (cur->name == "ustaw")
        changed |= simplify_identity(cur->arg[1]);

    for (std::size_t i = 0; i < cur->child.size(); i++)
        changed |= identity_simplification_node(cur->child[i]);

    return changed;
}

bool Tree::identity_simplification()
{
    return identity_simplification_node(beg);
}

bool Tree::loop_unrolling_node(Node *cur, std::unordered_map<std::string, long long> &know)
{
    bool changed = false;

    auto clone_subtree = [](const Node *src, Node *parent, const auto &clone_ref) -> Node *
    {
        Node *copy = new Node(src->name, parent);
        copy->arg = src->arg;
        for (Node *child : src->child)
            copy->child.push_back(clone_ref(child, copy, clone_ref));
        return copy;
    };

    for (std::size_t i = 0; i < cur->child.size();)
    {
        Node *child = cur->child[i];

        if (child->name == "ustaw")
        {
            child->arg[1] = remove_spaces(child->arg[1]);
            if (count_opperands(child->arg[1]) == 0)
            {
                if (is_num(child->arg[1]))
                    know[child->arg[0]] = std::stoll(child->arg[1]);
                else if (know.count(child->arg[1]))
                    know[child->arg[0]] = know[child->arg[1]];
                else
                    know.erase(child->arg[0]);
            }
            else
                know.erase(child->arg[0]);
        }

        if (child->name == "wczytaj_ram" && !child->arg.empty())
            know.erase(child->arg[0]);
        else if (child->name == "wczytaj")
            for (const std::string &name : edited_variables(child->name, child->arg))
                know.erase(name);

        if (child->name == "punkt" || child->name == "idz" || child->name == "idzjezeli" || child->name == "wywolaj")
            know.clear();

        if (child->name != "dopoki")
        {
            std::unordered_map<std::string, long long> nested = know;
            changed |= loop_unrolling_node(child, nested);
            i++;
            continue;
        }

        std::string beg_expr, sign, end_expr;
        parse_binary(remove_spaces(child->arg[0]), beg_expr, sign, end_expr);

        bool reversed = false;
        std::string induction = beg_expr;
        std::string limit_expr = end_expr;
        if (is_num(beg_expr) && var_names.count(end_expr))
        {
            reversed = true;
            induction = end_expr;
            limit_expr = beg_expr;
        }

        long long start = 0, limit = 0;
        if (!var_names.count(induction) || !know.count(induction) || !is_num(limit_expr))
        {
            changed |= loop_unrolling_node(child, know);
            know.clear();
            i++;
            continue;
        }

        start = know[induction];
        limit = std::stoll(limit_expr);

        if (reversed)
        {
            if (sign == ">")
                sign = "<";
            else if (sign == ">=")
                sign = "<=";
            else if (sign == "<")
                sign = ">";
            else if (sign == "<=")
                sign = ">=";
        }

        long long step = 0;
        bool found_step = false;
        for (Node *body_child : child->child)
        {
            if (body_child->name != "ustaw" || body_child->arg[0] != induction)
                continue;

            ArtDelta art;
            if (parse_assignment_art(induction, body_child->arg[1], art) && art.delta != 0)
            {
                step = art.delta;
                found_step = true;
                break;
            }
        }

        int iterations = 0;
        if (!found_step || !predict_iterations(start, sign, limit, step, iterations))
        {
            changed |= loop_unrolling_node(child, know);
            know.clear();
            i++;
            continue;
        }

        auto &v = cur->child;
        std::vector<Node *> expanded;
        for (int repeat = 0; repeat < iterations; repeat++)
            for (Node *body_child : child->child)
                expanded.push_back(clone_subtree(body_child, cur, clone_subtree));

        v.erase(v.begin() + i);
        v.insert(v.begin() + i, expanded.begin(), expanded.end());
        delete child;

        know.clear();
        changed = true;
        i += expanded.size();
    }

    return changed;
}

bool Tree::loop_unrolling()
{
    std::unordered_map<std::string, long long> know;
    return loop_unrolling_node(beg, know);
}

bool Tree::assignment_combining_node(Node *cur)
{
    bool changed = false;

    for (std::size_t i = 0; i < cur->child.size(); i++)
        changed |= assignment_combining_node(cur->child[i]);

    auto &v = cur->child;
    for (std::size_t i = 0; i < v.size();)
    {
        Node *first = v[i];
        if (first->name != "ustaw")
        {
            i++;
            continue;
        }

        ArtDelta art;
        if (!parse_assignment_art(first->arg[0], first->arg[1], art) || art.base != first->arg[0])
        {
            i++;
            continue;
        }

        std::size_t j = i + 1;
        long long total_delta = art.delta;
        while (j < v.size() && v[j]->name == "ustaw" && v[j]->arg[0] == first->arg[0])
        {
            ArtDelta next;
            if (!parse_assignment_art(first->arg[0], v[j]->arg[1], next) || next.base != first->arg[0])
                break;

            total_delta += next.delta;
            j++;
        }

        if (j == i + 1)
        {
            i++;
            continue;
        }

        art.delta = total_delta;
        first->arg[1] = art_to_expr(art);

        for (std::size_t erase_idx = i + 1; erase_idx < j; erase_idx++)
            delete v[erase_idx];
        v.erase(v.begin() + i + 1, v.begin() + j);

        changed = true;
        i++;
    }

    return changed;
}

bool Tree::assignment_combining()
{
    return assignment_combining_node(beg);
}

bool Tree::array_access_lowering_node(Node *cur, const std::unordered_map<std::string, std::string> &array_types)
{
    bool changed = false;

    auto make_node = [](const std::string &name, Node *parent, const std::vector<std::string> &arg)
    {
        Node *node = new Node(name, parent);
        node->arg = arg;
        if (name == "stworz" && arg.size() >= 2)
        {
            if (arg[0] == "tablica" && arg.size() >= 4)
                remember_variable_type(arg.back(), "tablica_" + arg[1]);
            else
                remember_variable_type(arg.back(), arg[0]);
        }
        return node;
    };

    auto materialize_index = [&](const std::string &index_expr, Node *parent, std::vector<Node *> &prefix)
    {
        std::string index_name = get_var_name("liczba");
        prefix.push_back(make_node("stworz", parent, {"liczba", index_name}));
        prefix.push_back(make_node("ustaw", parent, {index_name, index_expr}));
        return index_name;
    };

    auto lower_array_reads = [&](std::string expr, Node *parent, std::vector<Node *> &prefix)
    {
        expr = remove_spaces(expr);

        ArrayRef ref;
        while (find_array_ref(expr, array_types, ref))
        {
            std::string index_name = materialize_index(ref.index, parent, prefix);
            std::string value_name = get_var_name(array_types.at(ref.name));

            prefix.push_back(make_node("stworz", parent, {array_types.at(ref.name), value_name}));
            prefix.push_back(make_node("wczytaj_ram", parent, {value_name, ref.name, index_name}));

            expr.replace(ref.begin, ref.end - ref.begin, value_name);
        }

        return expr;
    };

    auto lower_node = [&](Node *node, std::vector<Node *> &prefix)
    {
        if (node->name == "ustaw" && node->arg.size() >= 2)
        {
            std::string rhs = lower_array_reads(node->arg[1], node->parent, prefix);

            ArrayRef target;
            if (parse_full_array_ref(node->arg[0], array_types, target))
            {
                std::string index_name = materialize_index(target.index, node->parent, prefix);
                std::string value_name = get_var_name(array_types.at(target.name));

                prefix.push_back(make_node("stworz", node->parent, {array_types.at(target.name), value_name}));
                prefix.push_back(make_node("ustaw", node->parent, {value_name, rhs}));

                node->name = "zapisz";
                node->arg = {value_name, target.name, index_name};
                return true;
            }

            if (rhs != remove_spaces(node->arg[1]))
            {
                node->arg[1] = rhs;
                return true;
            }
        }
        else if ((node->name == "zakoncz" || node->name == "idzjezeli") && !node->arg.empty())
        {
            std::string arg = lower_array_reads(node->arg[0], node->parent, prefix);
            if (arg != remove_spaces(node->arg[0]))
            {
                node->arg[0] = arg;
                return true;
            }
        }

        return !prefix.empty();
    };

    for (std::size_t i = 0; i < cur->child.size(); i++)
    {
        changed |= array_access_lowering_node(cur->child[i], array_types);

        std::vector<Node *> prefix;
        if (lower_node(cur->child[i], prefix))
            changed = true;

        if (!prefix.empty())
        {
            cur->child.insert(cur->child.begin() + i, prefix.begin(), prefix.end());
            i += prefix.size();
        }
    }

    return changed;
}

bool Tree::array_access_lowering()
{
    std::unordered_map<std::string, std::string> array_types;

    auto collect = [&](Node *node, const auto &self) -> void
    {
        if (node->name == "stworz" && node->arg.size() >= 4 && node->arg[0] == "tablica")
            array_types[node->arg.back()] = node->arg[1];

        for (Node *child : node->child)
            self(child, self);
    };

    collect(beg, collect);
    if (array_types.empty())
        return false;

    return array_access_lowering_node(beg, array_types);
}

void Tree::collect_variable_lifetimes_node(Node *cur, std::unordered_map<std::string, std::vector<std::string>> &type_by_name, std::unordered_map<std::string, Node *> &create_by_name, std::unordered_map<std::string, Node *> &last_use)
{
    if (cur->name == "stworz" && cur->arg.size() >= 2)
    {
        const std::string &name = cur->arg.back();
        type_by_name[name] = std::vector<std::string>(cur->arg.begin(), cur->arg.end() - 1);
        create_by_name[name] = cur;
    }

    for (const auto &name : used_variables(cur->name, cur->arg))
        last_use[name] = cur;
    for (const auto &name : edited_variables(cur->name, cur->arg))
        last_use[name] = cur;

    for (Node *child : cur->child)
        collect_variable_lifetimes_node(child, type_by_name, create_by_name, last_use);
}

void Tree::release_dead_variables_node()
{
    std::unordered_map<std::string, std::vector<std::string>> type_by_name;
    std::unordered_map<std::string, Node *> create_by_name;
    std::unordered_map<std::string, Node *> last_use;

    collect_variable_lifetimes_node(beg, type_by_name, create_by_name, last_use);

    auto is_ancestor = [](const Node *ancestor, const Node *node)
    {
        for (const Node *cur = node; cur != nullptr; cur = cur->parent)
            if (cur == ancestor)
                return true;
        return false;
    };

    auto safe_target = [&](Node *use, Node *create)
    {
        Node *target = use;
        for (Node *control = use; control != nullptr; control = control->parent)
            if ((control->name == "idzjezeli" || control->name == "dopoki") && !is_ancestor(control, create))
                target = control;
        return target;
    };

    auto next_is_release = [](Node *target, const std::string &name)
    {
        Node *par = target->parent;
        if (par == nullptr)
            return false;

        std::size_t idx = 0;
        while (idx < par->child.size() && par->child[idx] != target)
            idx++;

        return idx + 1 < par->child.size() && par->child[idx + 1]->name == "usun" && par->child[idx + 1]->arg.size() >= 2 && par->child[idx + 1]->arg.back() == name;
    };

    auto collect_idz_labels = [](Node *node, std::vector<std::string> &labels, const auto &self) -> void
    {
        if (node->name == "idz" && !node->arg.empty())
            labels.push_back(node->arg[0]);

        for (Node *child : node->child)
            self(child, labels, self);
    };

    auto child_index = [](Node *parent, Node *node)
    {
        Node *cur = node;
        while (cur->parent != nullptr && cur->parent != parent)
            cur = cur->parent;

        if (cur->parent != parent)
            return parent->child.size();

        std::size_t idx = 0;
        while (idx < parent->child.size() && parent->child[idx] != cur)
            idx++;
        return idx;
    };

    auto created_in_loop_header = [&](Node *target, Node *create)
    {
        if (target->name != "idzjezeli" || target->parent == nullptr || is_ancestor(target, create))
            return false;

        std::vector<std::string> labels;
        collect_idz_labels(target, labels, collect_idz_labels);
        if (labels.empty())
            return false;

        Node *par = target->parent;
        std::size_t target_idx = child_index(par, target);
        std::size_t create_idx = child_index(par, create);
        if (target_idx == par->child.size() || create_idx == par->child.size())
            return false;

        for (const std::string &label : labels)
        {
            for (std::size_t point_idx = 0; point_idx < target_idx; point_idx++)
                if (par->child[point_idx]->name == "punkt" && !par->child[point_idx]->arg.empty() && par->child[point_idx]->arg[0] == label)
                    return point_idx <= create_idx && create_idx < target_idx;
        }

        return false;
    };

    std::vector<std::string> names;
    for (const auto &entry : type_by_name)
        names.push_back(entry.first);
    std::sort(names.begin(), names.end());

    std::unordered_map<Node *, std::vector<std::string>> releases_by_target;
    for (const std::string &name : names)
    {
        if (!create_by_name.count(name) || !last_use.count(name))
            continue;

        Node *use = last_use[name];
        if (use->name == "zakoncz")
            continue;

        Node *target = safe_target(use, create_by_name[name]);
        if (target == nullptr || target->parent == nullptr || next_is_release(target, name))
            continue;
        if (created_in_loop_header(target, create_by_name[name]))
            continue;

        releases_by_target[target].push_back(name);
    }

    for (auto &entry : releases_by_target)
    {
        Node *target = entry.first;
        Node *par = target->parent;
        auto &v = par->child;

        std::size_t idx = 0;
        while (idx < v.size() && v[idx] != target)
            idx++;
        if (idx == v.size())
            continue;

        std::size_t offset = 1;
        for (const std::string &name : entry.second)
        {
            Node *release = new Node("usun", par);
            // release->arg.insert(release->arg.end(), type_by_name[name].begin(), type_by_name[name].end());
            release->arg.push_back(name);
            v.insert(v.begin() + idx + offset, release);
            offset++;
        }
    }
}

void Tree::release_dead_variables()
{
    release_dead_variables_node();
}

void Tree::varible_counter_node(Node *cur, std::unordered_map<std::string, int> &total_usage, std::unordered_map<std::string, int> &total_edits)
{
    if (cur->name == "ustaw")
    {
        std::vector<std::string> vars;
        add_expr_variables(cur->arg[1], vars);
        add_variable_counts(vars, total_usage);

        vars.clear();
        add_expr_variables(cur->arg[0], vars);
        const std::string target_base = first_variable(cur->arg[0]);
        for (const std::string &name : vars)
            if (name != target_base)
                total_usage[name]++;
    }

    if (cur->name == "idzjezeli" || cur->name == "zakoncz" || cur->name == "wczytaj_ram" || cur->name == "zapisz" || cur->name == "wypisz")
        add_variable_counts(used_variables(cur->name, cur->arg), total_usage);
    add_variable_counts(edited_variables(cur->name, cur->arg), total_edits);

    for (std::size_t i = 0; i < cur->child.size(); i++)
        varible_counter_node(cur->child[i], total_usage, total_edits);
}

bool Tree::varible_cleaner_node(Node *cur, std::unordered_map<std::string, int> &total_usage, std::unordered_map<std::string, int> &total_edits)
{
    bool changed = false;

    for (std::size_t i = 0; i < cur->child.size(); i++)
        changed |= varible_cleaner_node(cur->child[i], total_usage, total_edits);

    bool remove_assignment = false;
    if (cur->name == "ustaw")
    {
        if (var_names.count(cur->arg[0]))
            remove_assignment = total_usage[cur->arg[0]] == 0;
        else
        {
            const std::string target_base = first_variable(cur->arg[0]);
            remove_assignment = !target_base.empty() && total_usage[target_base] == 0;
        }
    }
    else if (cur->name == "wczytaj_ram" && !cur->arg.empty())
    {
        remove_assignment = total_usage[cur->arg[0]] == 0;
    }

    if (remove_assignment)
    {
        Node *par = cur->parent;
        std::size_t idx = 0;
        while (idx < par->child.size() && par->child[idx] != cur)
            idx++;

        par->child.erase(par->child.begin() + idx);

        delete cur;
        changed = 1;
    }
    if (cur->name == "stworz" && total_usage[cur->arg.back()] == 0 && total_edits[cur->arg.back()] == 0)
    {
        Node *par = cur->parent;
        std::size_t idx = 0;
        while (idx < par->child.size() && par->child[idx] != cur)
            idx++;

        par->child.erase(par->child.begin() + idx);

        delete cur;
        changed = 1;
    }

    return changed;
}

bool Tree::varible_cleaner()
{
    std::unordered_map<std::string, int> total_usage;
    std::unordered_map<std::string, int> total_edits;
    varible_counter_node(beg, total_usage, total_edits);
    return varible_cleaner_node(beg, total_usage, total_edits);
}

bool Tree::neg_simplification_node(Node *cur, std::unordered_map<std::string, Node *> &last_use)
{
    bool changed = false;

    if (cur->name == "ustaw")
    {
        std::string s_1 = cur->arg[1], beg_1, sign_1, end_1;
        split(s_1, beg_1, sign_1, end_1);

        if (sign_1 == "!" && cur->arg[0] == end_1 && last_use.count(end_1))
        {
            Node *par = cur->parent;
            std::size_t idx = 0;
            while (idx < par->child.size() && par->child[idx] != cur)
                idx++;

            Node *prv = last_use[end_1];

            if (prv->name == "ustaw" && prv->arg[0] == end_1)
            {
                std::string s_2 = prv->arg[1], beg_2, sign_2, end_2;
                split(s_2, beg_2, sign_2, end_2);
                bool can_rewrite = true;
                if (sign_2.empty())
                {
                    prv->arg[1] = "!" + remove_spaces(s_2);
                }
                else if (negation.count(sign_2))
                {
                    if (sign_2 == "&&" || sign_2 == "||")
                    {
                        beg_2 = "!(" + beg_2 + ")";
                        end_2 = "!(" + end_2 + ")";
                    }
                    sign_2 = negation[sign_2];
                    prv->arg[1] = beg_2 + sign_2 + end_2;
                }
                else
                {
                    can_rewrite = false;
                }

                if (can_rewrite)
                {
                    par->child.erase(par->child.begin() + idx);
                    delete cur;

                    return true;
                }
            }
        }
        else
        {
            last_use[cur->arg[0]] = cur;
            last_use[beg_1] = cur;
            last_use[end_1] = cur;
        }
    }
    else
        for (auto &s : cur->arg)
            last_use[s] = cur;

    for (std::size_t i = 0; i < cur->child.size(); i++)
        changed |= neg_simplification_node(cur->child[i], last_use);

    return changed;
}

bool Tree::neg_simplification()
{
    std::unordered_map<std::string, Node *> last_use;

    return neg_simplification_node(beg, last_use);
}

void Tree::print() const
{
    beg->print(0);
}

std::vector<Token> Tree::get_token() const
{
    std::vector<Token> tok;
    beg->get_tokens(tok);
    return tok;
}

Tree::Node *Tree::get_beg() const
{
    return beg;
}
