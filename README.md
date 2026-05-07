# Simple Compiler

This project is a refactored implementation of a simple compiler written in C++.

The compiler translates a small custom language into assembly-like instructions. The main goal of this version was to clean up the project structure, separate responsibilities between components, and add basic control-flow instructions such as conditional blocks and jumps.

# Project Structure

```text
.
├── main.cpp                            # Program entry point
├── token.hpp / token.cpp               # Command-line and token parsing
├── variable.hpp / variable.cpp         # Variable model
├── asm_command.hpp / asm_command.cpp   # Representation of ASM instructions
├── compiler.hpp / compiler.cpp         # Main compiler logic
└── CMakeLists.txt                      # Build configuration
```
# Building

```
cmake -S . -B build
cmake --build build
```

# Commands

The toy language uses Polish-inspired commands.

```text
-zakoncz <exit_code>
    Exit the program with the specified exit code.

-stworz <type> <name>
    Create a variable with the given type and name.

    Supported types:
    - liczba    4-byte integer
    - olbrzym   8-byte integer

-ustaw <name> <expression>
    Assign the value of <expression> to the variable <name>.

-jezeli <expression> begin
    Execute the following block only if <expression> is positive.

-end
    End the nearest block opened with begin.

-punkt <name>
    Define a label with the given name.

-idz <name>
    Jump to the label with the given name.
```

# Example codes

### 1. Basic arithmetic and variable assignment

```text
stworz liczba a
stworz liczba b
ustaw a 2+3
ustaw b a*2+3
zakoncz b
```

Expected result:
```text
exit code: 13
```

Explanation:
```text
a = 2 + 3 = 5
b = a * 2 + 3 = 13
```

### 2. Nested conditional blocks

This program demonstrates nested `jezeli ... begin ... end` blocks.

```text
stworz liczba a
stworz liczba b
ustaw a 3
ustaw b 0

jezeli a-1 begin
    ustaw b 5

    jezeli b-6 begin
        ustaw b 9
    end
end

zakoncz b
```

Expected result:
```text
exit code: 5
```

Explanation:
```text
a - 1 = 2, so the first block is executed.
b is set to 5.
b - 6 = -1, so the nested block is not executed.
The program exits with b = 5.
```

### 3. Loop using labels and jumps

This program computes 5! using a `label` and a `jump`.

```text
stworz liczba i
stworz liczba a
ustaw i 5
ustaw a 1

punkt loop
jezeli i-1 begin
    ustaw a a*i
    ustaw i i-1
    idz loop
end

zakoncz a
```

Expected result:
```text
exit code: 120
```
Explanation:
```text
The program repeatedly multiplies a by i and decreases i by 1.
It computes:

5 * 4 * 3 * 2 * 1 = 120
```

# What I Learned

This project helped me understand how higher-level control-flow constructs can be translated into lower-level jump instructions.

The most interesting part was handling nested conditional blocks correctly: each end has to match the nearest currently open begin, which required keeping track of block structure during compilation.

# Technologies

-C++

-CMake

-Compiler design basics

-Control-flow compilation
