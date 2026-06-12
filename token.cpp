#include "token.hpp"

#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace
{
    const std::unordered_map<std::string, int> lim = {
        {"stworz", 2},
        {"ustaw", 2},
        {"begin", 0},
        {"end", 0},
        {"dopoki", 1},
        {"jezeli", 1},
        {"albojezeli", 1},
        {"albo", 0},
        {"punkt", 1},
        {"idz", 1},
        {"wczytaj", 1},
        {"wypisz", 1},
        {"zapisz", 3},
        {"funkcja", 2},
        {"zakoncz", 1}};

    const std::unordered_set<std::string> logi = {"dopoki", "jezeli", "albojezeli"};
    const std::unordered_set<std::string> arit = {"ustaw", "zakoncz"};
    const std::unordered_set<std::string> text = {"wypisz", "wczytaj"};
    const std::unordered_set<std::string> it_depends = {"stworz"};
    const std::unordered_set<std::string> value_types = {"liczba", "logika", "adres"};

    const std::unordered_map<std::string, int> type_arg = {{"liczba", 1}, {"logika", 1}, {"adres", 1}, {"tablica", 3}};

    std::vector<std::string> split(const std::string &text)
    {
        std::stringstream ss(text);

        std::vector<std::string> res;
        std::string word;

        while (ss >> word)
        {
            res.push_back(word);
        }

        return res;
    }
}

Token::Token() {}

Token::Token(const std::string &s)
{
    const auto v = split(s);

    if (v.empty())
    {
        name = "Pusty";
    }
    else
    {
        name = v[0];

        auto it = lim.find(name);

        if (it == lim.end())
            throw std::runtime_error("Nieznana komenda: " + name);

        int expectedArgs = it->second;
        int actualArgs = static_cast<int>(v.size()) - 1;

        if (name == "funkcja")
        {
            if (actualArgs < 2)
                throw std::runtime_error("Zla liczba argumentow funkcji");
            if ((actualArgs - 2) % 2 != 0)
                throw std::runtime_error("Niepelna para typu i nazwy argumentu funkcji: " + v[2]);
            if ((actualArgs - 2) / 2 > 4)
                throw std::runtime_error("Za duzo argumentow w funkcji: " + v[2]);
            if (!value_types.count(v[1]))
                throw std::runtime_error("Nie istniejacy typ zwracany funkcji: " + v[1]);
            for (std::size_t i = 3; i < v.size(); i += 2)
                if (!value_types.count(v[i]))
                    throw std::runtime_error("Nie istniejacy typ argumentu funkcji: " + v[i]);
            actualArgs = expectedArgs;
        }

        if (logi.count(name))
        {
            actualArgs = expectedArgs;
            arg.emplace_back();
            for (std::size_t i = 1; i < v.size(); i++)
            {
                arg[0] += v[i] + ' ';
            }
            arg[0].pop_back();
        }

        if (arit.count(name))
        {
            actualArgs = expectedArgs;
            std::size_t expr_begin = 2;
            if (name == "zakoncz")
            {
                arg.emplace_back();
                expr_begin = 1;
            }
            else
            {
                arg.push_back(v[1]);
                arg.emplace_back();
            }

            if (v.size() <= expr_begin)
                throw std::runtime_error("Brak wyrazenia: " + name);

            for (std::size_t i = expr_begin; i < v.size(); i++)
            {
                if (!arg.back().empty())
                    arg.back() += ' ';
                arg.back() += v[i];
            }
        }

        if (it_depends.count(name))
        {
            if (actualArgs < 2)
                throw std::runtime_error("Zła liczba argumentów " + name);
            if (type_arg.count(v[1]))
            {
                auto itr = type_arg.find(v[1]);
                if (actualArgs != itr->second + 1)
                    throw std::runtime_error("Zla liczba argumentow " + name);
                for (std::size_t i = 1; i < v.size(); i++)
                    arg.push_back(v[i]);
                actualArgs = expectedArgs;
            }
            else
                throw std::runtime_error("Nie istniejący typ: " + v[1]);
        }

        if (text.count(name))
        {
            actualArgs = expectedArgs;

            arg.emplace_back();

            for (std::size_t i = 1; i < v.size(); i++)
                arg[0] += v[i] + ' ';
            arg[0].pop_back();
        }

        if (actualArgs != expectedArgs)
            throw std::runtime_error("Zla liczba arugmentow: " + name);

        if (!logi.count(name) && !arit.count(name) && !it_depends.count(name) && !text.count(name))
            for (std::size_t i = 1; i < v.size(); i++)
            {
                arg.push_back(v[i]);
            }
    }
}
