# Aether Programming Language

A systems language with Python-style syntax and C performance, featuring automatic type inference and actor-based concurrency.

## Overview

Aether aims to combine the ergonomics of dynamic languages with the performance of compiled systems languages. Code is written without explicit type annotations, and the compiler infers types using Hindley-Milner style inference before generating optimized C code.

```aether
x = 42
name = "Alice"
nums = [1, 2, 3]

main() {
    print(x)
}
```

The above compiles to typed C code with no runtime overhead.

## Key Features

- Full type inference (no type annotations required)
- Python-style syntax without `let` or type declarations
- Compiles to readable C code
- Actor-based concurrency model
- Zero-cost abstractions

## Comparison with C

| Feature | Aether | C |
|---------|--------|---|
| Type annotations | Optional | Required |
| Syntax | Minimal | Verbose |
| Memory safety | Bounds checking available | Manual |
| Concurrency | Built-in actors | Manual threads |
| String handling | First-class types | Manual char* |
| Performance | Compiles to C | Native |

## Current Status

**Phase 2 Complete**

The compiler is functional and passes all current test cases:
- Compiler builds and runs on Windows/Cygwin
- Type inference working for primitives, arrays, structs, and functions
- Python-style syntax parsing (no explicit types needed)
- Code generation produces valid C code
- Control flow structures (if/while/for) supported
- Actor syntax parses and compiles

**Test Coverage:** 20 passing examples

**Next Phase:** Implement actor runtime (spawn/send primitives), improve type inference edge cases, expand test coverage.

## Building

### Requirements
- GCC (via Cygwin, MSYS2, or native on Linux/Mac)
- Make or PowerShell

### Windows (PowerShell)
```powershell
.\build_compiler.ps1
```

### Linux/Mac
```bash
make
```

## Usage

Compile an Aether program to C:
```powershell
.\build\aetherc.exe program.ae output.c
```

Compile the generated C to executable:
```bash
gcc output.c -Iruntime runtime/*.c -o program
./program
```

## Examples

### Type Inference
```aether
x = 42          // inferred as int
pi = 3.14       // inferred as float
name = "Alice"  // inferred as string
nums = [1, 2, 3] // inferred as int[3]

main() {
    print(x)
}
```

### Functions
```aether
add(a, b) {
    return a + b
}

main() {
    result = add(10, 20)
    print(result)
}
```

### Structs
```aether
struct Point {
    x,
    y
}

main() {
    p = Point{ x: 10, y: 20 }
    print(p.x)
}
```

### Actors (syntax supported, runtime in progress)
```aether
actor Counter {
    state count = 0
    
    receive(msg) {
        count = count + 1
    }
}

main() {
    print("Actor defined")
}
```

## Known Limitations

- Actor spawn/send runtime not yet implemented
- Pattern matching (match statements) not implemented
- Some edge cases in type inference
- Limited standard library
- No package system yet

## Project Structure

- `compiler/` - Lexer, parser, type checker, code generator
- `runtime/` - Runtime library (strings, memory, actors)
- `examples/` - Example programs and tests
- `docs/` - Language specification and guides

## Documentation

- `docs/language-reference.md` - Language specification
- `docs/TYPE_INFERENCE_GUIDE.md` - Type system details
- `docs/RUNTIME_GUIDE.md` - Runtime API

## License

MIT
