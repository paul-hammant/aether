# Aether Examples

Test programs and examples demonstrating language features.

## Directory Structure

### basic/
Simple introductory examples:
- `hello_world.ae` - Minimal hello world program
- `hello_actors.ae` - Basic actor definition

### tests/
Feature-specific test cases:
- `test_arrays.ae` - Array syntax and operations
- `test_condition.ae` - If statements
- `test_for_loop.ae` - Loop constructs
- `test_struct.ae` - Basic struct usage
- `test_struct_complex.ae` - Nested structs and arrays
- `test_actor_simple.ae` - Actor definition
- `test_multiple_actors.ae` - Multiple actor definitions
- `test_modern_syntax.ae` - Python-style syntax

### advanced/
More complex examples:
- `main_example.ae` - Multiple features combined
- `supervisor.ae` - Multiple actors with state

### benchmarks/
Performance testing examples:
- `simple_demo.ae` - Basic feature demo
- `working_demo.ae` - Type inference demo
- `performance_demo.ae` - Computation loop
- `feature_showcase.ae` - Multiple types
- `ring_benchmark.ae` - Actor ring topology

### Root Level
- `minimal_syntax.ae` - Demonstrates type inference with structs
- `type_inference.ae` - Type inference examples
- `runtime_test.ae` - Runtime library features

## Running Examples

### Compile to C
```bash
.\build\aetherc.exe examples\basic\hello_world.ae build\hello.c
```

### Compile C to Executable
```bash
gcc build\hello.c -Iruntime runtime\*.c -o hello.exe
```

### Run
```bash
.\hello.exe
```

## Current Status

All examples compile successfully and generate valid C code. Most examples run correctly, though some features (actor spawn/send, pattern matching) are syntax-only at this stage.

## Testing

Run all examples:
```powershell
.\test_all_files.ps1
```

This script compiles each .ae file to C, then compiles the C code, reporting any failures.

## Notes

Examples focus on currently implemented features:
- Type inference (full support)
- Structs and arrays (working)
- Functions (working)
- Control flow (working)
- Actors (syntax only, runtime in progress)

Examples avoid unimplemented features like pattern matching (match statements) and runtime actor operations (spawn/send).
