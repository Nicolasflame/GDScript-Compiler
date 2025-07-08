# GDScript Compiler

A direct-to-machine-code compiler for GDScript, the scripting language used in the Godot game engine. This compiler translates GDScript source code directly into native machine code without transpiling to another high-level language.

## Features

### Core Compilation Pipeline
- **Lexical Analysis**: Tokenizes GDScript source code with support for all GDScript syntax
- **Syntax Analysis**: Builds Abstract Syntax Tree (AST) using recursive descent parsing
- **Semantic Analysis**: Performs type checking, symbol resolution, and semantic validation
- **Code Generation**: Generates native machine code with register allocation and optimization

### Language Support
- **Data Types**: `int`, `float`, `String`, `bool`, `Array`, `Dictionary`, `Variant`
- **Control Flow**: `if/else`, `while`, `for`, `match` statements
- **Functions**: Function declarations with parameters and return types
- **Classes**: Class definitions with inheritance, methods, and properties
- **Signals**: GDScript signal declarations and handling
- **Built-in Functions**: `print()`, `len()`, `range()`, `str()`, `int()`, `float()`

### Advanced Features
- **Type Inference**: Automatic type deduction where possible
- **Register Allocation**: Efficient register usage with virtual register support
- **Code Optimization**: Dead code elimination and constant folding
- **Error Reporting**: Comprehensive error messages with line numbers
- **Cross-Platform**: Supports x86_64 and ARM64 architectures

## Project Structure

```
GDScript Compiler/
├── main.cpp              # Main compiler entry point
├── lexer.h/.cpp          # Lexical analyzer
├── parser.h/.cpp         # Syntax analyzer and AST builder
├── semantic_analyzer.h/.cpp # Semantic analysis and type checking
├── code_generator.h/.cpp # Machine code generation
├── examples/             # Example GDScript files
├── Makefile              # Build system
├── clean.sh              # Cleanup script
└── README.md             # This file
```

## Building the Compiler

### Prerequisites
- C++17 compatible compiler (GCC 7+ or Clang 5+)
- Make build system
- POSIX-compliant operating system (Linux, macOS, BSD)

### Build Commands

```bash
# Build the compiler
make

# Clean build artifacts
make clean

# Debug build with symbols
make debug

# Optimized release build
make release

# Run basic tests
make test

# Install to system path
make install
```

## Usage

### Basic Compilation

```bash
# Compile GDScript file to machine code
./bin/gdscript-compiler input.gd output

# This generates:
# - output.s   (assembly code)
# - output.o   (object file)
# - output     (executable)
```

### Example GDScript Code

See the `examples/` directory for sample GDScript files that can be compiled with this compiler.

### Command Line Options

```bash
Usage: gdscript-compiler [options] <input.gd> <output>

Options:
  -h, --help     Show help message
  -v, --version  Show compiler version
  -O, --optimize Enable optimizations
  -g, --debug    Include debug information
  -S, --asm-only Generate assembly only
  -c, --compile  Generate object file only
```

## Architecture

### Compilation Phases

1. **Lexical Analysis** (`lexer.cpp`)
   - Tokenizes source code into meaningful symbols
   - Handles keywords, operators, literals, and identifiers
   - Manages indentation-based syntax

2. **Syntax Analysis** (`parser.cpp`)
   - Builds Abstract Syntax Tree (AST)
   - Implements recursive descent parser
   - Handles operator precedence and associativity

3. **Semantic Analysis** (`semantic_analyzer.cpp`)
   - Type checking and inference
   - Symbol table management
   - Scope resolution
   - Error detection

4. **Code Generation** (`code_generator.cpp`)
   - Intermediate representation (IR) generation
   - Register allocation
   - Machine code emission
   - Optimization passes

## Platform Support

### Supported Architectures
- **x86_64**: Intel/AMD 64-bit processors
- **ARM64**: Apple Silicon and ARM64 processors

### Operating Systems
- **Linux**: All major distributions
- **macOS**: Intel and Apple Silicon Macs
- **Windows**: PE executable format support

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

## License

This project is open source. See the LICENSE file for details.