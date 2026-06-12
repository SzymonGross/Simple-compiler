#pragma once

#include <string>

enum class Typ
{
    adres,
    liczba,
    logika,
    tablica_adres,
    tablica_liczba,
    tablica_logika
};

struct Varbile
{
    std::string name;
    Typ type;

    std::size_t size = 0;
    std::size_t ele_size = 0;

    std::size_t addres;

    static Typ convert(std::string s);

    Varbile(const std::string &name, const std::string &type) : name(name), type(convert(type)) {}
    Varbile(const std::string &type) : type(convert(type)) {}
    Varbile(const Typ type) : type(type) {}
};