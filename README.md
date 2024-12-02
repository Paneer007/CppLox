# CppLox - Bytecode Interpreter 

**CppLox** is a dynamically typed, object-oriented bytecode interpreter engineered in C++. 

The project has taken inspiration from the book Crafting interpreters

The project involves parsing source code, generating bytecode instructions, and executing them efficiently with a stack-based virtual machine. It also includes features like automatic memory management and performance optimization techniques.

## Features

- **Dynamically Typed, Object-Oriented Interpreter**: Executes flexible and complex programs using bytecode.
- **Top-Down Pratt Parser**: Recursively parses source code, ensuring syntactic and semantic consistency, and generates bytecode.
- **Stack-Based Virtual Machine**: Processes bytecode instructions, supporting arithmetic, logical, control flow operations, and function invocations.
- **Garbage Collection**: Implements a mark and sweep garbage collector for automatic memory management.
- **Efficient Data Structures**: Uses **Tries** for faster tokenization and **closed-addressing Hash Tables** for global variables, string interning, and classes.
- **Performance Optimizations**: Enhances performance through **NaN boxing**, **backpatching**, and **arithmetic optimizations**.


# Building and installing

See the [BUILDING](BUILDING.md) document.

# Contributing

See the [CONTRIBUTING](CONTRIBUTING.md) document.

# Licensing

FCAlertView is available under the MIT license. See the LICENSE file for more info.

