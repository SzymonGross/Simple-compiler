#include "mask.hpp"

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <iostream>

namespace
{
    const std::unordered_set<std::string> types = {"logika", "liczba", "adres"};

    const std::unordered_set<std::string> operands = {"+", "-", "*", "/", "%", "<<", ">>", "&&", "||", "==", "!=", "<=", ">=", "<", ">", "!"};

    std::vector<std::string> split(const std::string &s)
    {
        std::string buf = "";
        std::vector<std::string> res;
        for (char c : s)
        {

            if (c == '{' || c == '}')
            {
                if (!buf.empty())
                    res.push_back(buf);
                res.push_back(std::string() + c);
                buf = "";
            }
            else if (buf.back() == '<' && c == '-')
            {
                buf.pop_back();
                if (!buf.empty())
                    res.push_back(buf);
                res.push_back("<-");
                buf = "";
            }
            else if (c == ' ')
            {
                if (!buf.empty())
                    res.push_back(buf);
                buf = "";
            }
            else
                buf += c;
        }

        if (!buf.empty())
            res.push_back(buf);
        return res;
    }

    bool dec(const std::string &s)
    {
        std::string buf = "";
        for (char c : s)
        {
            buf += c;
            if (types.count(buf))
                return true;
        }
        return false;
    }

    int arrow(const std::vector<std::string> &vs)
    {
        for (int i = 0; i < vs.size(); i++)
            if (vs[i] == "<-")
                return i;
        return -1;
    }

    bool conteins(const std::string &s, char c)
    {
        for (const auto wc : s)
            if (wc == c)
                return true;
        return false;
    }

    bool has_char_before(const std::vector<std::string> &vs, char c, int end)
    {
        if (end == -1)
            end = static_cast<int>(vs.size());

        for (int i = 1; i < end; i++)
            if (conteins(vs[i], c))
                return true;
        return false;
    }

    std::vector<std::string> declaration_names(const std::vector<std::string> &vs, int end)
    {
        if (end == -1)
            end = static_cast<int>(vs.size());

        std::vector<std::string> names;
        std::string buf;

        for (int i = 1; i < end; i++)
        {
            for (char c : vs[i])
            {
                if (c == ',')
                {
                    if (!buf.empty())
                        names.push_back(buf);
                    buf = "";
                }
                else
                    buf += c;
            }

            if (!buf.empty())
            {
                names.push_back(buf);
                buf = "";
            }
        }

        return names;
    }
}

std::istringstream mask(std::istream &input)
{
    std::ostringstream out;
    std::string line;

    while (std::getline(input, line))
    {
        std::vector<std::string> content = split(line);

        if (content.empty())
            continue;

        bool beg = 0, end = 0;
        if (content.back() == "{")
        {
            content.pop_back();
            beg = 1;
        }
        if (content[0] == "}")
        {
            end = 1;
            content.erase(content.begin());
        }

        if (end)
            out << "end\n";

        bool did_anyting = 0;

        if (content[0] == "poczontek")
        {
            did_anyting = 1;
            out << "funkcja liczba main\n";
        }

        if (dec(content[0]))
        {
            did_anyting = 1;
            int arrow_idx = arrow(content);
            if (conteins(content[0], '['))
            {
                std::string a = "", b = "";
                bool is = 0;
                for (char c : content[0])
                {
                    if (c == '[' || c == ']')
                        is = 1;
                    else if (is)
                        b += c;
                    else
                        a += c;
                }

                out << "stworz tablica " << a << " " << b << " " << content[1] << "\n";
            }
            else if (has_char_before(content, '(', arrow_idx))
            {
                std::vector<std::string> arg;
                std::string buf;
                for (const auto &word : content)
                {
                    for (const auto c : word)
                    {
                        if (c == '(' || c == ')' || c == ',' || c == ' ')
                        {
                            if (!buf.empty())
                                arg.push_back(buf);
                            buf = "";
                        }
                        else
                            buf += c;
                    }
                    if (!buf.empty())
                        arg.push_back(buf);
                    buf = "";
                }

                out << "funkcja ";
                for (const auto &word : arg)
                    out << word << " ";
                out << "\n";
            }
            else
            {
                for (const std::string &name : declaration_names(content, arrow_idx))
                    out << "stworz " << content[0] << " " << name << "\n";
            }
        }

        if (arrow(content) != -1)
        {
            did_anyting = 1;
            int idx = arrow(content);
            if (idx == 0)
                throw std::runtime_error("<- w złym miejscu");

            out << "ustaw " << content[idx - 1] << " ";

            if (operands.count(content[idx + 1].substr(0, 1)))
            {
                std::string opp = content[idx + 1].substr(0, 1);
                content[idx + 1].erase(content[idx + 1].begin());

                out << content[idx - 1] << opp;
            }
            else if (content[idx + 1].size() > 1 && operands.count(content[idx + 1].substr(0, 2)))
            {
                std::string opp = content[idx + 1].substr(0, 2);
                content[idx + 1].erase(content[idx + 1].begin());
                content[idx + 1].erase(content[idx + 1].begin());

                out << content[idx - 1] << opp;
            }

            for (int i = idx + 1; i < content.size(); i++)
                out << content[i];
            out << "\n";
        }

        if (!content.empty() && !did_anyting)
        {
            for (const auto &s : content)
                out << s << " ";
            out << "\n";
        }

        if (beg)
            out << "begin\n";
    }

    return std::istringstream(out.str());
}
