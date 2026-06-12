#include "varible.hpp"

#include <stdexcept>

Typ Varbile::convert(const std::string s)
{
    if (s == "adres")
        return Typ::adres;
    if (s == "liczba")
        return Typ::liczba;
    if (s == "logika")
        return Typ::logika;
    if (s == "tablica_adres")
        return Typ::tablica_adres;
    if (s == "tablica_liczba")
        return Typ::tablica_liczba;
    if (s == "tablica_logika")
        return Typ::tablica_logika;

    throw std::invalid_argument("Nieznany typ: " + s);
}