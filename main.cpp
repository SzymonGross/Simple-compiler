#include "compiler.hpp"

#include <fstream>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Uzycie: " << argv[0] << " <wejscie> <wyjscie>\n";
        return 1;
    }

    std::ifstream input(argv[1]);
    if (!input) {
        std::cerr << "Nie mozna otworzyc pliku wejsciowego\n";
        return 1;
    }

    std::ofstream output(argv[2]);
    if (!output) {
        std::cerr << "Nie mozna otworzyc pliku wyjsciowego\n";
        return 1;
    }

    try {
        Compiler compiler(input);
        compiler.write(output);
    } catch (const std::exception& e) {
        std::cerr << "Blad: " << e.what() << '\n';
        return 1;
    }

    return 0;
}