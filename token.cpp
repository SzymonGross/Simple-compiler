#include "token.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace {

std::string trimLeft(std::string s) {
    auto it = std::find_if_not(s.begin(), s.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    s.erase(s.begin(), it);
    return s;
}

std::string trimRight(std::string s) {
    auto it = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    s.erase(it.base(), s.end());
    return s;
}

std::string trim(std::string s) {
    return trimRight(trimLeft(std::move(s)));
}

const std::unordered_map<std::string, int> kCommandId{
    {"zakoncz", 1},
    {"stworz", 2},
    {"ustaw", 3},
    {"wypisz", 4},
    {"jezeli", 5},
    {"end", 6},
    {"punkt", 7},
    {"idz", 8},
};

}  // namespace

Token parseLine(const std::string& line) {
    std::string cleaned = trim(line);
    if (cleaned.empty()) {
        return Token{};
    }

    std::istringstream iss(cleaned);
    std::string command;
    iss >> command;
    if (command.empty()) {
        return Token{};
    }

    auto it = kCommandId.find(command);
    if (it == kCommandId.end()) {
        throw std::invalid_argument("Nieznana komenda: " + command);
    }

    Token token;
    token.id = it->second;

    if (command == "stworz") {
        std::string typeName;
        std::string varName;
        if (!(iss >> typeName >> varName)) {
            throw std::invalid_argument("Komenda 'stworz' wymaga 2 argumentow");
        }
        token.args = {typeName, varName};
        return token;
    }

    if (command == "ustaw") {
        std::string target;
        if (!(iss >> target)) {
            throw std::invalid_argument("Komenda 'ustaw' wymaga nazwy zmiennej docelowej");
        }
        std::string expr;
        std::getline(iss, expr);
        expr = trim(expr);
        if (expr.empty()) {
            throw std::invalid_argument("Komenda 'ustaw' wymaga wyrazenia");
        }
        token.args = {target, expr};
        return token;
    }

    if (command == "zakoncz" || command == "wypisz") {
        std::string expr;
        std::getline(iss, expr);
        expr = trim(expr);
        if (expr.empty()) {
            throw std::invalid_argument("Komenda wymaga argumentu");
        }
        token.args = {expr};
        return token;
    }

    if (command == "jezeli") {
        std::string rest;
        std::getline(iss, rest);
        rest = trim(rest);
        const std::string suffix = " begin";
        if (rest.size() <= suffix.size() || rest.compare(rest.size() - suffix.size(), suffix.size(), suffix) != 0) {
            throw std::invalid_argument("Komenda 'jezeli' ma postac: jezeli <wyrazenie> begin");
        }

        std::string expr = trim(rest.substr(0, rest.size() - suffix.size()));
        if (expr.empty()) {
            throw std::invalid_argument("Komenda 'jezeli' wymaga wyrazenia");
        }
        token.args = {expr};
        return token;
    }

    if (command == "end") {
        std::string extra;
        std::getline(iss, extra);
        if (!trim(extra).empty()) {
            throw std::invalid_argument("Komenda 'end' nie przyjmuje argumentow");
        }
        return token;
    }

    if (command == "punkt" || command == "idz") {
        std::string name;
        if (!(iss >> name)) {
            throw std::invalid_argument("Komenda '" + command + "' wymaga nazwy punktu");
        }
        std::string extra;
        std::getline(iss, extra);
        if (!trim(extra).empty()) {
            throw std::invalid_argument("Komenda '" + command + "' przyjmuje dokladnie 1 argument");
        }
        token.args = {name};
        return token;
    }

    throw std::logic_error("Nieobslugiwany przypadek parsowania");
}
