#include "compiler.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace
{

    const std::unordered_map<std::string, int> kTypeId{
        {"liczba", 1},
        {"olbrzym", 2},
    };

    const std::unordered_map<std::string, int> kPriority{
        {"*", 2},
        {"+", 1},
        {"-", 1},
        {"(", 0},
        {")", 0},
    };

    bool isSpace(unsigned char ch)
    {
        return std::isspace(ch) != 0;
    }

} // namespace

Compiler::Compiler(std::istream &input)
{
    processProgram(input);
    peepholeOptimize();
}

void Compiler::processProgram(std::istream &input)
{
    std::string line;
    while (std::getline(input, line))
    {
        Token token = parseLine(line);
        if (token.id != 0)
        {
            handleToken(token);
        }
    }

    if (!ifBlocks_.empty())
    {
        throw std::logic_error("Brakujace 'end' dla instrukcji 'jeżeli ... begin'");
    }

    for (const std::string &name : referencedPoints_)
    {
        if (definedPoints_.find(name) == definedPoints_.end())
        {
            throw std::logic_error("Skok 'idz' do niezdefiniowanego punktu: " + name);
        }
    }
}

void Compiler::handleToken(const Token &token)
{
    switch (token.id)
    {
    case 1:
        handleEnd(token);
        break;
    case 2:
        handleCreate(token);
        break;
    case 3:
        handleSet(token);
        break;
    case 4:
        handlePrint(token);
        break;
    case 5:
        handleIf(token);
        break;
    case 6:
        handleBlockEnd(token);
        break;
    case 7:
        handlePoint(token);
        break;
    case 8:
        handleJump(token);
        break;
    default:
        throw std::logic_error("Nieznany identyfikator tokena");
    }
}

Compiler::VarPtr Compiler::createGlobal(const std::string &typeName, const std::string &name)
{
    assertUserNameAvailable(name, "zmiennej");

    if (globals_.find(name) != globals_.end())
    {
        throw std::logic_error("Zmienna zostala zadeklarowana wczesniej: " + name);
    }

    auto typeIt = kTypeId.find(typeName);
    if (typeIt == kTypeId.end())
    {
        throw std::logic_error("Nieznany typ: " + typeName);
    }

    const int type = typeIt->second;
    auto variable = std::make_unique<Variable>(name, type, static_cast<std::ptrdiff_t>(nextOffset_), false);
    if (type == 1)
    {
        nextOffset_ += 4;
    }
    else if (type == 2)
    {
        nextOffset_ += 8;
    }
    maxMemoryUsed_ = std::max(maxMemoryUsed_, nextOffset_);

    VarPtr ptr = variable.get();
    globals_.emplace(name, std::move(variable));
    return ptr;
}

Compiler::VarPtr Compiler::createTemp(int type)
{
    while (globals_.find("R" + std::to_string(tempCounter_)) != globals_.end())
    {
        ++tempCounter_;
    }

    const std::string name = "R" + std::to_string(tempCounter_++);
    auto variable = std::make_unique<Variable>(name, type);
    variable->temporary = true;

    if (type == 1)
    {
        variable->memoryAddress = static_cast<std::ptrdiff_t>(nextOffset_);
        nextOffset_ += 4;
    }
    else if (type == 2)
    {
        variable->memoryAddress = static_cast<std::ptrdiff_t>(nextOffset_);
        nextOffset_ += 8;
    }

    maxMemoryUsed_ = std::max(maxMemoryUsed_, nextOffset_);
    temporaries_.push_back(std::move(variable));
    return temporaries_.back().get();
}

Compiler::VarPtr Compiler::createConstant(const std::string &value)
{
    auto variable = std::make_unique<Variable>(value, 0);
    variable->temporary = true;
    temporaries_.push_back(std::move(variable));
    return temporaries_.back().get();
}

bool Compiler::isNumber(const std::string &text) const
{
    return !text.empty() && std::all_of(text.begin(), text.end(), [](unsigned char ch)
                                        { return std::isdigit(ch) != 0; });
}

bool Compiler::isOperator(const std::string &text) const
{
    return text == "+" || text == "-" || text == "*";
}

std::string Compiler::readOperator(const std::string &text, std::size_t index) const
{
    for (std::size_t len = 1; len <= 1; ++len)
    {
        if (index + len <= text.size())
        {
            std::string sub = text.substr(index, len);
            if (isOperator(sub) || sub == "(" || sub == ")")
            {
                return sub;
            }
        }
    }
    return {};
}

std::vector<std::string> Compiler::toRpn(const std::string &expression) const
{
    std::vector<std::string> output;
    std::vector<std::string> ops;
    std::string buffer;

    for (std::size_t i = 0; i < expression.size();)
    {
        if (isSpace(static_cast<unsigned char>(expression[i])))
        {
            if (!buffer.empty())
            {
                output.push_back(buffer);
                buffer.clear();
            }
            ++i;
            continue;
        }

        std::string op = readOperator(expression, i);
        if (!op.empty())
        {
            if (!buffer.empty())
            {
                output.push_back(buffer);
                buffer.clear();
            }

            if (op == "(")
            {
                ops.push_back(op);
            }
            else if (op == ")")
            {
                while (!ops.empty() && ops.back() != "(")
                {
                    output.push_back(ops.back());
                    ops.pop_back();
                }
                if (ops.empty())
                {
                    throw std::logic_error("Blad nawiasow w wyrazeniu: " + expression);
                }
                ops.pop_back();
            }
            else
            {
                while (!ops.empty() && ops.back() != "(" && kPriority.at(ops.back()) >= kPriority.at(op))
                {
                    output.push_back(ops.back());
                    ops.pop_back();
                }
                ops.push_back(op);
            }
            ++i;
        }
        else
        {
            buffer.push_back(expression[i++]);
        }
    }

    if (!buffer.empty())
    {
        output.push_back(buffer);
    }

    while (!ops.empty())
    {
        if (ops.back() == "(" || ops.back() == ")")
        {
            throw std::logic_error("Blad nawiasow w wyrazeniu: " + expression);
        }
        output.push_back(ops.back());
        ops.pop_back();
    }

    return output;
}

Compiler::VarPtr Compiler::resolveOperand(const std::string &token)
{
    if (isNumber(token))
    {
        return createConstant(token);
    }

    for (auto &temp : temporaries_)
    {
        if (temp->name == token)
        {
            return temp.get();
        }
    }

    auto g = globals_.find(token);
    if (g != globals_.end())
    {
        return g->second.get();
    }

    throw std::logic_error("Nieznany identyfikator: " + token);
}

std::string Compiler::operandText(const Variable *var) const
{
    if (var->type == 0)
    {
        return var->name;
    }
    if (var->type == 1)
    {
        return "DWORD PTR [rbp-" + std::to_string(var->memoryAddress) + "]";
    }
    if (var->type == 2)
    {
        return "QWORD PTR [rbp-" + std::to_string(var->memoryAddress) + "]";
    }
    throw std::logic_error("Niepoprawny typ zmiennej dla: " + var->name + " [" + (char)(var->type + '0') + "]");
}

std::string Compiler::registerName(std::size_t size) const
{
    if (size == 4)
    {
        return "eax";
    }
    if (size == 8)
    {
        return "rax";
    }
    throw std::logic_error("Nieobslugiwany rozmiar rejestru");
}

std::size_t Compiler::resultSize(const Variable *lhs, const Variable *rhs, const Variable *dst) const
{
    std::size_t size = 0;
    if (lhs)
        size = std::max(size, lhs->size);
    if (rhs)
        size = std::max(size, rhs->size);
    if (dst)
        size = std::max(size, dst->size);
    if (size == 0)
    {
        size = 4;
    }
    if (size != 4 && size != 8)
    {
        throw std::logic_error("Nieobslugiwany rozmiar operacji");
    }
    return size;
}

void Compiler::emitLoadToRegister(const Variable *source, std::size_t regSize)
{
    body_.emplace_back(Op::Mov, registerName(regSize), operandText(source));
}

void Compiler::emitStoreFromRegister(const Variable *destination, std::size_t regSize)
{
    body_.emplace_back(Op::Mov, operandText(destination), registerName(regSize));
}

void Compiler::emitMove(const Variable *destination, const Variable *source)
{
    if (destination->type == 0)
    {
        throw std::logic_error("Nie mozna przypisac do stalej");
    }

    if (destination->type != 0 && source->type != 0)
    {
        const std::size_t size = resultSize(destination, source);
        emitLoadToRegister(source, size);
        emitStoreFromRegister(destination, size);
        return;
    }

    body_.emplace_back(Op::Mov, operandText(destination), operandText(source));
}

void Compiler::emitBinary(const std::string &op, const Variable *lhs, const Variable *rhs, const Variable *destination)
{
    const std::size_t size = resultSize(lhs, rhs, destination);
    const std::string reg = registerName(size);

    emitLoadToRegister(lhs, size);

    if (op == "+")
    {
        body_.emplace_back(Op::Add, reg, operandText(rhs));
    }
    else if (op == "-")
    {
        body_.emplace_back(Op::Sub, reg, operandText(rhs));
    }
    else if (op == "*")
    {
        body_.emplace_back(Op::Imul, reg, operandText(rhs));
    }
    else
    {
        throw std::logic_error("Nieznany operator: " + op);
    }

    if (destination)
    {
        emitStoreFromRegister(destination, size);
    }
}

Compiler::VarPtr Compiler::evaluateExpression(const std::string &expression, VarPtr output)
{
    const std::vector<std::string> rpn = toRpn(expression);
    std::vector<VarPtr> stack;

    for (std::size_t i = 0; i < rpn.size(); ++i)
    {
        const std::string &token = rpn[i];

        if (!isOperator(token))
        {
            stack.push_back(resolveOperand(token));
            continue;
        }

        if (stack.size() < 2)
        {
            throw std::logic_error("Niepoprawne wyrazenie RPN");
        }

        VarPtr rhs = stack.back();
        stack.pop_back();
        VarPtr lhs = stack.back();
        stack.pop_back();

        VarPtr dst = nullptr;
        const bool lastOperation = (i + 1 == rpn.size());
        if (lastOperation && output != nullptr)
        {
            dst = output;
        }
        else
        {
            dst = createTemp(resultSize(lhs, rhs) / 4);
        }

        emitBinary(token, lhs, rhs, dst);
        stack.push_back(dst);
    }

    if (stack.size() != 1)
    {
        throw std::logic_error("Niepoprawne wyrazenie");
    }

    VarPtr result = stack.back();
    if (output != nullptr && result != output)
    {
        emitMove(output, result);
        result = output;
    }

    return result;
}

void Compiler::clearTemporaries()
{
    temporaries_.clear();
}

std::size_t Compiler::alignStackSize(std::size_t usedBytes)
{
    return ((usedBytes / 16) + 1) * 16 + 32;
}

void Compiler::handleCreate(const Token &token)
{
    if (token.args.size() != 2)
    {
        throw std::logic_error("Komenda 'stworz' wymaga 2 argumentow");
    }
    createGlobal(token.args[0], token.args[1]);
}

void Compiler::handleSet(const Token &token)
{
    if (token.args.size() != 2)
    {
        throw std::logic_error("Komenda 'ustaw' wymaga 2 argumentow");
    }

    auto it = globals_.find(token.args[0]);
    if (it == globals_.end())
    {
        throw std::logic_error("Nieznana zmienna docelowa: " + token.args[0]);
    }

    evaluateExpression(token.args[1], it->second.get());
    clearTemporaries();
}

void Compiler::handleEnd(const Token &token)
{
    if (token.args.size() != 1)
    {
        throw std::logic_error("Komenda 'zakoncz' wymaga 1 argumentu");
    }

    VarPtr result = createTemp(1);
    evaluateExpression(token.args[0], result);

    const std::size_t stackSize = alignStackSize(maxMemoryUsed_);

    header_.emplace_back(Op::Directive, ".intel_syntax noprefix");
    header_.emplace_back(Op::Directive, ".global main");
    header_.emplace_back(Op::Label, "main:");
    header_.emplace_back(Op::Push, "rbp");
    header_.emplace_back(Op::Mov, "rbp", "rsp");
    header_.emplace_back(Op::Sub, "rsp", std::to_string(stackSize));

    footer_.emplace_back(Op::Mov, "eax", operandText(result));
    footer_.emplace_back(Op::Add, "rsp", std::to_string(stackSize));
    footer_.emplace_back(Op::Pop, "rbp");
    footer_.emplace_back(Op::Ret);

    clearTemporaries();
}


void Compiler::validateUserName(const std::string &name, const std::string &kind) const
{
    if (name.empty())
    {
        throw std::logic_error("Pusta nazwa " + kind);
    }

    const auto isFirst = [](unsigned char ch) {
        return std::isalpha(ch) != 0 || ch == '_';
    };
    const auto isNext = [](unsigned char ch) {
        return std::isalnum(ch) != 0 || ch == '_';
    };

    if (!isFirst(static_cast<unsigned char>(name.front())))
    {
        throw std::logic_error("Niepoprawna nazwa " + kind + ": " + name);
    }

    for (unsigned char ch : name)
    {
        if (!isNext(ch))
        {
            throw std::logic_error("Niepoprawna nazwa " + kind + ": " + name);
        }
    }

    static const std::unordered_set<std::string> reserved{
        "zakoncz", "stworz", "ustaw", "wypisz", "jeżeli", "jezeli", "begin", "end", "punkt", "idz"
    };
    if (reserved.find(name) != reserved.end())
    {
        throw std::logic_error("Nazwa " + kind + " koliduje ze slowem kluczowym: " + name);
    }
}

void Compiler::assertUserNameAvailable(const std::string &name, const std::string &kind) const
{
    validateUserName(name, kind);

    if (globals_.find(name) != globals_.end())
    {
        throw std::logic_error("Nazwa " + kind + " koliduje z istniejaca zmienna: " + name);
    }
    if (points_.find(name) != points_.end())
    {
        throw std::logic_error("Nazwa " + kind + " koliduje z istniejacym punktem: " + name);
    }
}

std::string Compiler::nextPointLabel(const std::string &name)
{
    return ".L_" + name + "_" + std::to_string(pointCounter_++);
}

std::string Compiler::nextIfEndLabel()
{
    return ".L_if_end_" + std::to_string(ifCounter_++) + ":";
}

void Compiler::handlePrint(const Token &token)
{
    if (token.args.size() != 1)
    {
        throw std::logic_error("Komenda 'wypisz' wymaga 1 argumentu");
    }

    // Miejsce na dalsza rozbudowe: np. wywolanie printf albo zapis do bufora.
    // Na ten moment zachowujemy komendę jako punkt rozszerzenia.
}


void Compiler::handleIf(const Token &token)
{
    if (token.args.size() != 1)
    {
        throw std::logic_error("Komenda 'jeżeli' wymaga 1 argumentu");
    }

    VarPtr result = createTemp(1);
    evaluateExpression(token.args[0], result);

    const std::string label = nextIfEndLabel();
    body_.emplace_back(Op::Cmp, operandText(result), "0");
    body_.emplace_back(Op::Jle, label.substr(0, label.size() - 1));
    ifBlocks_.push_back({label});

    clearTemporaries();
}

void Compiler::handleBlockEnd(const Token &token)
{
    if (!token.args.empty())
    {
        throw std::logic_error("Komenda 'end' nie przyjmuje argumentow");
    }
    if (ifBlocks_.empty())
    {
        throw std::logic_error("Nadmiarowy 'end' bez odpowiadajacego 'jeżeli ... begin'");
    }

    body_.emplace_back(Op::Label, ifBlocks_.back().endLabel);
    ifBlocks_.pop_back();
}

void Compiler::handlePoint(const Token &token)
{
    if (token.args.size() != 1)
    {
        throw std::logic_error("Komenda 'punkt' wymaga 1 argumentu");
    }

    const std::string &name = token.args[0];
    validateUserName(name, "punktu");

    if (globals_.find(name) != globals_.end())
    {
        throw std::logic_error("Nazwa punktu koliduje ze zmienna: " + name);
    }
    if (definedPoints_.find(name) != definedPoints_.end())
    {
        throw std::logic_error("Punkt zostal zdefiniowany wczesniej: " + name);
    }

    auto [it, inserted] = points_.emplace(name, std::string{});
    if (inserted || it->second.empty())
    {
        it->second = nextPointLabel(name);
    }

    definedPoints_.insert(name);
    body_.emplace_back(Op::Label, it->second + ":");
}

void Compiler::handleJump(const Token &token)
{
    if (token.args.size() != 1)
    {
        throw std::logic_error("Komenda 'idz' wymaga 1 argumentu");
    }

    const std::string &name = token.args[0];
    validateUserName(name, "punktu");

    if (globals_.find(name) != globals_.end())
    {
        throw std::logic_error("Nazwa punktu w 'idz' koliduje ze zmienna: " + name);
    }

    auto [it, inserted] = points_.emplace(name, std::string{});
    if (inserted || it->second.empty())
    {
        it->second = nextPointLabel(name);
    }

    referencedPoints_.insert(name);
    body_.emplace_back(Op::Jmp, it->second);
}

std::size_t Compiler::totalSize() const
{
    return header_.size() + body_.size() + footer_.size();
}

AsmCommand &Compiler::lineAt(std::size_t index)
{
    if (index < header_.size())
    {
        return header_[index];
    }

    index -= header_.size();
    if (index < body_.size())
    {
        return body_[index];
    }

    index -= body_.size();
    if (index < footer_.size())
    {
        return footer_[index];
    }

    throw std::out_of_range("Niepoprawny indeks linii ASM");
}

const AsmCommand &Compiler::lineAt(std::size_t index) const
{
    if (index < header_.size())
    {
        return header_[index];
    }

    index -= header_.size();
    if (index < body_.size())
    {
        return body_[index];
    }

    index -= body_.size();
    if (index < footer_.size())
    {
        return footer_[index];
    }

    throw std::out_of_range("Niepoprawny indeks linii ASM");
}

void Compiler::eraseLine(std::size_t index)
{
    if (index < header_.size())
    {
        header_.erase(header_.begin() + static_cast<std::ptrdiff_t>(index));
        return;
    }

    index -= header_.size();
    if (index < body_.size())
    {
        body_.erase(body_.begin() + static_cast<std::ptrdiff_t>(index));
        return;
    }

    index -= body_.size();
    if (index < footer_.size())
    {
        footer_.erase(footer_.begin() + static_cast<std::ptrdiff_t>(index));
        return;
    }

    throw std::out_of_range("Niepoprawny indeks linii ASM");
}

void Compiler::peepholeOptimize()
{
    for (std::size_t i = 0; i + 1 < totalSize();)
    {
        AsmCommand &cur = lineAt(i);
        AsmCommand &next = lineAt(i + 1);

        if (cur.op == Op::Mov && cur.a == cur.b)
        {
            eraseLine(i);
            continue;
        }

        if (cur.op == Op::Mov && next.op == Op::Mov && cur.a == next.a)
        {
            eraseLine(i);
            continue;
        }

        if (cur.op == Op::Mov && next.op == Op::Add && isNumber(cur.b) && isNumber(next.b) && cur.a == next.a)
        {
            cur.b = std::to_string(std::stoll(cur.b) + std::stoll(next.b));
            eraseLine(i + 1);
            continue;
        }

        ++i;
    }
}

void Compiler::write(std::ostream &output) const
{
    std::size_t indent = 0;
    const std::size_t count = header_.size() + body_.size() + footer_.size();

    for (std::size_t i = 0; i < count; ++i)
    {
        const AsmCommand &cmd = lineAt(i);
        for (std::size_t j = 0; j < indent; ++j)
        {
            output << ' ';
        }
        output << cmd.emit() << '\n';
        if (cmd.op == Op::Label)
        {
            indent += 4;
        }
    }
}
