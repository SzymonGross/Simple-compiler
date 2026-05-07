#pragma once

#include "asm_command.hpp"
#include "token.hpp"
#include "variable.hpp"

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Compiler {
public:
    explicit Compiler(std::istream& input);
    void write(std::ostream& output) const;

private:
    using VarPtr = Variable*;

    struct IfBlock {
        std::string endLabel;
    };

    void processProgram(std::istream& input);
    void handleToken(const Token& token);

    VarPtr createGlobal(const std::string& typeName, const std::string& name);
    VarPtr createTemp(int type);
    VarPtr createConstant(const std::string& value);

    bool isNumber(const std::string& text) const;
    bool isOperator(const std::string& text) const;
    std::string readOperator(const std::string& text, std::size_t index) const;
    std::vector<std::string> toRpn(const std::string& expression) const;
    VarPtr resolveOperand(const std::string& token);
    std::string operandText(const Variable* var) const;
    std::string registerName(std::size_t size) const;
    std::size_t resultSize(const Variable* lhs, const Variable* rhs, const Variable* dst = nullptr) const;

    void emitLoadToRegister(const Variable* source, std::size_t regSize);
    void emitStoreFromRegister(const Variable* destination, std::size_t regSize);
    void emitMove(const Variable* destination, const Variable* source);
    void emitBinary(const std::string& op, const Variable* lhs, const Variable* rhs, const Variable* destination);

    VarPtr evaluateExpression(const std::string& expression, VarPtr output = nullptr);
    void clearTemporaries();
    static std::size_t alignStackSize(std::size_t usedBytes);

    void handleCreate(const Token& token);
    void handleSet(const Token& token);
    void handleEnd(const Token& token);
    void handlePrint(const Token& token);
    void handleIf(const Token& token);
    void handleBlockEnd(const Token& token);
    void handlePoint(const Token& token);
    void handleJump(const Token& token);

    std::size_t totalSize() const;
    AsmCommand& lineAt(std::size_t index);
    const AsmCommand& lineAt(std::size_t index) const;
    void eraseLine(std::size_t index);
    void peepholeOptimize();

    std::string nextIfEndLabel();
    std::string nextPointLabel(const std::string& name);
    void validateUserName(const std::string& name, const std::string& kind) const;
    void assertUserNameAvailable(const std::string& name, const std::string& kind) const;

    std::unordered_map<std::string, std::unique_ptr<Variable>> globals_;
    std::vector<std::unique_ptr<Variable>> temporaries_;
    std::vector<AsmCommand> header_;
    std::vector<AsmCommand> body_;
    std::vector<AsmCommand> footer_;
    std::vector<IfBlock> ifBlocks_;
    std::unordered_map<std::string, std::string> points_;
    std::unordered_set<std::string> definedPoints_;
    std::unordered_set<std::string> referencedPoints_;

    std::size_t nextOffset_ = 4;
    std::size_t maxMemoryUsed_ = 0;
    int tempCounter_ = 0;
    std::size_t ifCounter_ = 0;
    std::size_t pointCounter_ = 0;
};
