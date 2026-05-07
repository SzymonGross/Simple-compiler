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

# Building

```
cmake -S . -B build
cmake --build build
```

```markdown
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

# What I Learned

This project helped me understand how higher-level control-flow constructs can be translated into lower-level jump instructions.

The most interesting part was handling nested conditional blocks correctly: each end has to match the nearest currently open begin, which required keeping track of block structure during compilation.

# Technologies

-C++

-CMake

-Compiler design basics

-Control-flow compilation