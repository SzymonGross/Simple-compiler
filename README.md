# Simple compiler

A simple compiler written in C++17. It translates a small Polish-inspired
source language from `.pl` files into x86-64 assembly using Intel syntax
(`.intel_syntax noprefix`).

The current version supports variables, arrays, arithmetic and logical
expressions, conditionals, loops, functions, recursion, and several tree-level
and assembly-level optimizations.

## Project Structure

```text
.
├── main.cpp
├── compiler.hpp / compiler.cpp         # Main compilation pipeline
├── mask.hpp / mask.cpp                 # Converts convenient .pl syntax into tokens
├── token.hpp / token.cpp               # Token parsing
├── tree.hpp / tree.cpp                 # Program tree and high-level optimizations
├── asm_command.hpp / asm_command.cpp   # Assembly generation and peephole optimization
├── varible.hpp / varible.cpp           # Variable model
├── kody_pl/                            # Example source programs
├── kody_s/                             # Generated assembly output
└── CMakeLists.txt
```

## Building

```bash
cmake -S . -B build
cmake --build build --target komp
```

## Usage

```bash
./build/komp <input_file.pl> <output_file.s>
```

Example:

```bash
./build/komp kody_pl/p5.pl kody_s/p5.s
```

The generated assembly can be assembled with GCC:

```bash
gcc kody_s/p5.s -o p5
./p5
```

## Language Syntax

The program entry point should be written as a `poczontek` block. Functions can
be defined before `poczontek`.

```text
poczontek {
    liczba x <- 7
    zakoncz x
}
```

### Types

```text
liczba      # 32-bit integer
logika      # boolean value, stored as one byte
```

### Variables and Assignments

```text
liczba a
liczba b <- 12
logika ok <- a < b

a <- b + 3
a <-+ 1      # shorthand for: a <- a + 1
a <-/ 10     # shorthand for: a <- a / 10
```

Internally, these instructions are lowered into `stworz` and `ustaw` tokens.

### Arrays

```text
liczba[15] tab
tab[0] <- 1
tab[i+2] <- tab[i] + tab[i+1]
```

Array accesses are lowered into internal `wczytaj` and `zapisz` operations.

### Conditionals

```text
jezeli a < b {
    a <- b
}
albojezeli a == b {
    a <- 0
}
albo {
    a <- a - b
}
```

### Loops

```text
dopoki n > 0 {
    suma <-+ n % 10
    n <-/ 10
}
```

### Functions

A function has the following form:

```text
<return_type> <name> (<arg_type_1> <arg_name_1>, ...)
{
    zakoncz <expression>
}
```

Up to 4 arguments are supported. Function arguments are passed through `ecx`,
`edx`, `r8d`, `r9d` for `liczba`, and `cl`, `dl`, `r8b`, `r9b` for `logika`.

Recursive example:

```text
liczba fun (liczba m, liczba n)
{
    jezeli m == 0 {
        zakoncz n + 1
    }
    albojezeli n == 0 {
        zakoncz fun(m-1, 1)
    }
    albo {
        zakoncz fun(m-1, fun(m,n-1))
    }
}

poczontek {
    zakoncz fun(3,3)
}
```

Function calls are lowered in `tree.cpp` into:

```text
ustaw <argument_register> <expression>
wywolaj <function_name>
ustaw <target> eax/al
```

For nested function calls, the compiler first materializes arguments into
temporary variables so that a `call` cannot overwrite already prepared argument
registers.

### Expressions

Supported operators:

```text
+  -  *  /  %  <<  >>
==  !=  <  <=  >  >=
&&  ||  !
```

Expressions can use parentheses, variables, constants, arrays, and function
calls:

```text
wynik <- (a + b) * 3
ok <- a < b || b == 5
x <- fun(a-1, tab[i])
```

### Returning / Exiting

```text
zakoncz <expression>
```

Inside a function, this sets the return value. Inside `poczontek`, it sets the
program exit code.

## Examples

### Fibonacci With an Array

```text
poczontek{
liczba[15] tab
tab[0] <- 1
tab[1] <- 1

liczba i <- 0

dopoki i < 5 {
    tab[i+2] <- tab[i] + tab[i+1]
    i <- i+1
}

zakoncz tab[6]
}
```

Expected exit code: `13`.

### Digit Sum and Conditionals

```text
poczontek{
liczba n <- 98765
liczba suma <- 0
liczba cyfra <- 0

dopoki n > 0 {
    cyfra <- n%10
    suma <-+ cyfra
    n <-/ 10
}

liczba wynik <- 0

jezeli suma < 10 {
    wynik <- 1
}
albojezeli suma < 25 {
    wynik <- 2
}
albojezeli suma < 40 {
    wynik <- 3
}
albo {
    wynik <- 4
}

zakoncz wynik
}
```

Expected exit code: `3`.

### Recursion

`kody_pl/p5.pl` computes `fun(3,3)` and should exit with code `61`.

## Optimizations

`tree.cpp` performs optimizations and lowering passes such as:

- constant folding,
- identity simplification,
- dead branch removal,
- simple loop unrolling,
- assignment combining,
- array access lowering,
- dead variable release,
- function call lowering into register setup and `wywolaj`.

`asm_command.cpp` performs peephole optimization, including dead store removal,
known register value folding, and collapsing sequences such as:

```asm
cmp dword ptr [rbp-8], 0
setne al
mov byte ptr [rbp-9], al
test al, al
jne endif_1
```

into:

```asm
cmp dword ptr [rbp-8], 0
jne endif_1
```

## Notes

- `kody_pl/p1.pl` is an older example without a `poczontek` block.
- Current examples using the newer syntax are mainly `p2.pl`, `p3.pl`, `p4.pl`,
  and `p5.pl`.
- The `varible` filename is historical and has been kept as-is.
