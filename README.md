# Simple Compiler

This project is a refactored implementation of a simple compiler written in C++.

The compiler translates a small Polish-inspired language into assembly-like
instructions. It supports variables, arithmetic expressions, logical
expressions, conditional blocks, loops, labels, jumps, and simple arrays.

The main goal of this version was to clean up the project structure, separate
responsibilities between components, and make higher-level control flow compile
down to lower-level jump instructions.

# Project Structure

```text
.
├── main.cpp                            # Program entry point
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
stworz <type> <name>
    Create a variable with the given type and name.

stworz tablica <type> <size> <name>
    Create an array with the given element type, size, and name.

    Supported types:
    - liczba    integer
    - logika    boolean/logical value

ustaw <name> <expression>
    Assign the value of <expression> to the variable or array element.

jezeli <expression>
begin
    Execute the following block only if <expression> is true.
end

dopoki <expression>
begin
    Repeat the following block while <expression> is true.
end

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
stworz liczba a
ustaw a 15

stworz liczba b
ustaw b 12

dopoki a!=b
begin
    jezeli a<b
    begin
        stworz liczba c
        ustaw c a
        ustaw a b
        ustaw b c
    end

    ustaw a a - b
end

zakoncz a
```

Expected result:

```text
exit code: 3
```

## p2.pl - Loop with logical condition

This program uses multiplication, a `dopoki` loop, comparison operators, and a
logical `||` condition.

```text
stworz liczba a
ustaw a 3

stworz liczba b
ustaw b 7

stworz liczba c
ustaw c a*b

dopoki b > 0
begin
    ustaw b b-1

    jezeli a > b || b == 5
    begin
        ustaw a a+1
    end
end

zakoncz a
```

Expected result:

```text
exit code: 8
```

## p3.pl - Fibonacci numbers with an array

This program stores Fibonacci numbers in an array and exits with `tab[6]`.

```text
stworz tablica liczba 15 tab
ustaw tab[0] 1
ustaw tab[1] 1

stworz liczba i
ustaw i 0

dopoki i < 5
begin
    ustaw tab[i+2] tab[i] + tab[i+1]
    ustaw i i+1
end

zakoncz tab[6]
```

Expected result:

```text
exit code: 13
```

# What I Learned

This project helped me understand how higher-level control-flow constructs can
be translated into lower-level jump instructions.

The most interesting part was handling nested blocks correctly: each `end` has
to match the nearest currently open `begin`, which requires keeping track of
block structure during compilation.

I also learned how expression parsing, temporary variables, arrays, and simple
optimizations affect the generated assembly.

# Technologies

- C++
- CMake
- Compiler design basics
- Control-flow compilation
