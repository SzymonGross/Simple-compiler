# Simple Compiler

This is a simple compiler written in C++.

The compiler translates a small Polish-inspired language into assembly-like
instructions. It supports variables, arithmetic expressions, logical
expressions, conditional blocks, loops, labels, jumps, and simple arrays.

The main goal of this version was to implement AST and idea of transleting language step by step to simpler intermidate language.

# Project Structure

```text
.
├── main.cpp                            # Program entry point
├── mask.cpp                            # Converts code from improved syntax to old one
├── token.hpp / token.cpp               # Command-line and token parsing
├── varible.hpp / varible.cpp           # Variable model
├── tree.hpp / tree.cpp                 # Syntax tree and transformations
├── asm_command.hpp / asm_command.cpp   # Representation and generation of ASM instructions
├── compiler.hpp / compiler.cpp         # Main compiler logic
├── kody_pl/                            # Example source programs
├── kody_s/                             # Example generated assembly output
└── CMakeLists.txt                      # Build configuration
```

# Building

```bash
cmake -S . -B build
cmake --build build
```

# Usage

```bash
./build/komp <input_file> <output_file>
```

Example:

```bash
./build/komp kody_pl/p1.pl kody_s/p1.s
```

# Commands

The toy language uses Polish-inspired commands.

```text
<type> <name>
    Create a variable with the given type and name.

<type> <name> <- <expresion>
    Create a variable with the given type and name, then assign the value of <expresion> to varible <name>.

<type>[<size>] <name>
    Create an array with the given element type, size, and name.

    Supported types:
    - liczba    integer
    - logika    boolean/logical value

<name> <- <expression>
    Assign the value of <expression> to the variable or array element.

jezeli <expression>
{
    Execute the following block only if <expression> is true.
}

albojezeli <expression>
{
    Execute the following block only if <expression> is true and previus jezeli was false.
}

albo
{
    Execute the following block only if all previus jezeli, albojezeli were false.
}

dopoki <expression>
{
    Repeat the following block while <expression> is true.
}

punkt <name>
    Define a label with the given name.

idz <name>
    Jump to the label with the given name.

zakoncz <expression>
    Exit the program with the value of <expression>.
```

Expressions can use arithmetic, comparisons, logic, and array indexing:

```text
+  -  *  /  %  <<  >>
==  !=  <  <=  >  >=
&&  ||  !
tab[index]
```

# Example Codes

## p1.pl - Greatest common divisor

This program computes the greatest common divisor of 15 and 12 using repeated
subtraction.

```text
liczba a <- 15

liczba b <- 12

dopoki a != b {
    jezeli a < b 
    {
        liczba c <- a
        a <- b
        b <- c
    }

    a <- a - b
}

zakoncz a
```

Expected result:

```text
exit code: 3
```

## p2.pl - Loop with logical condition

This program uses multiplication, a `dopoki` loop, comparison operators, and a
logical `||` condition. Compliator is able to optymalize it to single return statment.

```text
liczba a <- 3
liczba b <- 7

liczba c <- a*b

dopoki b > 0
{
    b <- b-1

    jezeli a > b || b == 5
    {
        a <- a+1
    }
}

zakoncz a
```

Expected result:

```text
exit code: 8
```

## p3.pl - Fibonacci numbers with an array

This program stores Fibonacci numbers in an array and exits with `tab[6]`. Compliator is able to optymalize it to single return statment.

```text
liczba[15] tab
tab[0] <- 1
tab[1] <- 1

liczba i <- 0

dopoki i < 5 {
    tab[i+2] <- tab[i] + tab[i+1]
    i <- i+1
}

zakoncz tab[6]
```

Expected result:

```text
exit code: 13
```

## p4.pl - Digit sum and nested if statments

This program computes sum of digits of number and then classyfies it to category.

```text
liczba n <- 98765
liczba suma <- 0

liczba cyfra <- 0

dopoki n > 0
{
    cyfra <- n%10
    suma <-+ cyfra
    n <-/ 10
}

liczba wynik <- 0

jezeli suma < 10{
    wynik <- 1
}
albojezeli suma < 25{
    wynik <- 2
}
albojezeli suma < 40 {
    wynik <- 3
}
albo{
    wynik <- 4
}

zakoncz wynik
```

Expected result:

```text
exit code: 3
```
