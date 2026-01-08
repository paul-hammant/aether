# Aether Test Suite

Professional, comprehensive test suite for Aether compiler, runtime, and standard library.

## Quick Start

```bash
# Run all tests (recommended)
./run_tests.ps1        # Windows PowerShell
./run_tests.sh         # Linux/macOS Bash

# Run specific test category
cd stdlib && ./run_tests.ps1    # Standard library tests
cd runtime && ./run_tests.sh    # Runtime tests
cd compiler && make test        # Compiler tests
```

## Test Organization

```
tests/
├── stdlib/              # Standard Library Tests (COMPLETE)
│   ├── test_framework.h     # Common test framework
│   ├── test_vector.c        # Vector collection (9 tests)
│   ├── test_hashmap.c       # HashMap collection (10 tests)
│   ├── test_set.c           # Set collection (6 tests)
│   ├── run_tests.ps1        # Windows runner
│   ├── run_tests.sh         # Unix runner
│   ├── Makefile             # Make support
│   └── README.md            # Detailed documentation
│
├── runtime/             # Runtime & Scheduler Tests
│   ├── test_scheduler_integration.c    # Multi-core scheduler
│   ├── test_runtime_implementations.c  # CPU, SIMD, Batching
│   ├── test_optimizations.c            # SIMD batching & coalescing
│   ├── test_harness.c                  # Test infrastructure
│   ├── test_runtime_*.c                # Stdlib runtime tests
│   └── ...
│
├── compiler/            # Compiler Tests
│   ├── test_lexer.c                    # Lexical analysis
│   ├── test_parser.c                   # Syntax parsing
│   ├── test_typechecker.c              # Type checking
│   ├── test_codegen.c                  # Code generation
│   └── ...
│
├── integration/         # Integration Tests
│   ├── runtime_test.ae                 # End-to-end Aether programs
│   └── ...
│
└── memory/             # Memory Management Tests
    └── ...
```

## Test Categories

### 1. Standard Library Tests - PRODUCTION-READY
- **Status:** Complete, 25/25 tests passing
- **Platform:** Windows, Linux, macOS
- **Coverage:** Vector, HashMap, Set
- **Framework:** Professional test framework with zero redundancy
- **Documentation:** [stdlib/README.md](stdlib/README.md)

**Quick Run:**
```bash
cd stdlib
./run_tests.ps1    # Windows
./run_tests.sh     # Unix
make test          # If make available
```

### 2. Runtime Tests
- **Focus:** Scheduler, SIMD, CPU detection, Batching
- **Files:** 11+ test files
- **Coverage:** Multi-core actor scheduler, runtime optimizations

### 3. Compiler Tests
- **Focus:** Lexer, Parser, Type checker, Code generator
- **Files:** 13+ test files
- **Coverage:** Full compiler pipeline

### 4. Integration Tests
- **Focus:** End-to-end Aether program compilation and execution
- **Files:** Aether source files (.ae)

### 5. Memory Tests
- **Focus:** Memory management, arena allocation, leak detection

## Running Tests

### All Tests (Master Runner)
```bash
# Windows
.\run_tests.ps1

# Linux/macOS
./run_tests.sh
```

### Standard Library Only (Recommended for Quick Validation)
```bash
cd stdlib
./run_tests.ps1    # Fastest, most comprehensive
```

### Runtime Tests
```bash
cd runtime
# Individual test compilation and execution
gcc -o test_scheduler test_scheduler_integration.c ../runtime/*.c -I.. -lpthread
./test_scheduler
```

### Compiler Tests
```bash
cd compiler
# Compile and run specific tests
gcc -o test_lexer test_lexer.c ../compiler/lexer.c -I..
./test_lexer
```

## Test Framework

### Standard Library Framework
Professional framework with:
- `TEST_SUITE_BEGIN(name)` - Initialize suite
- `TEST(name, condition)` - Run assertion
- `TEST_SUITE_END()` - Print results
- Common utilities: `test_int_hash()`, `test_str_hash()`, `test_make_int()`

See [stdlib/README.md](stdlib/README.md) for complete API.

### Runtime Test Harness
- `test_harness.h/c` - Runtime test infrastructure
- Macros: `TEST_PASS`, `TEST_FAIL`
- Support for async/concurrent tests

## Requirements

- **Compiler:** GCC or Clang with C11 support
- **OS:** Windows, Linux, or macOS
- **Optional:** 
  - Make utility for `make test`
  - pthread library for runtime tests
  - AVX2 support for SIMD tests (auto-detected)

## Test Status

| Category | Tests | Status | Coverage |
|----------|-------|--------|----------|
| **stdlib** | 25 | PASS | Vector, HashMap, Set |
| **runtime** | 11+ | 🔄 Active | Scheduler, SIMD, CPU |
| **compiler** | 13+ | 🔄 Active | Full pipeline |
| **integration** | Varies | 🔄 Active | End-to-end |
| **memory** | Varies | 🔄 Active | Allocation, leaks |

## CI/CD Integration

All test runners return proper exit codes:
- `0` - All tests passed
- `1` - One or more tests failed

### GitHub Actions Example
```yaml
- name: Run all tests
  run: |
    cd tests
    ./run_tests.sh
    
- name: Run stdlib tests only
  run: |
    cd tests/stdlib
    ./run_tests.sh
```

## Adding New Tests

### For Standard Library
1. Create test file in `tests/stdlib/`
2. Use `test_framework.h` macros
3. Update `run_tests.ps1` and `run_tests.sh`
4. See [stdlib/README.md](stdlib/README.md) for template

### For Runtime/Compiler
1. Create test file in appropriate directory
2. Use existing test harness or create standalone
3. Update master test runners if needed
4. Document in this README

## Directory Structure Benefits

**Clear separation** - Easy to find specific test types  
**Modular** - Each category can be tested independently  
**Scalable** - Easy to add new test categories  
**Professional** - Standard project organization  
**Maintainable** - Clear ownership and purpose  

## Documentation

- **Main README:** This file
- **stdlib/README.md:** Complete stdlib test documentation
- **stdlib/REFACTORING_SUMMARY.md:** Test improvements details

## Contributing

When adding tests:
1. Choose the appropriate directory (stdlib/runtime/compiler/integration/memory)
2. Follow existing patterns in that directory
3. Use the test framework if available
4. Update this README if adding a new category
5. Ensure cross-platform compatibility

## Test Organization

### Unit Tests
- `test_lexer.c` - Lexical analyzer tests
- `test_parser.c` - Parser tests
- `test_typechecker.c` - Type checker tests
- `test_codegen.c` - Code generator tests
- `test_structs.c` - Struct parsing and type checking tests

### Integration Tests
- `test_integration.c` - End-to-end compilation pipeline tests
- `test_examples.c` - Example program compilation and execution tests

### Test Infrastructure
- `test_harness.h` - Test framework header
- `test_harness.c` - Test framework implementation
- `test_main.c` - Test runner entry point

## Running Tests

### Prerequisites
- GCC compiler must be in PATH for integration tests
- Aether compiler must be built at `build/aetherc.exe`

### Build and Run
```bash
# Build test runner
gcc tests/test_*.c compiler/lexer.c compiler/parser.c compiler/ast.c compiler/typechecker.c compiler/codegen.c -Icompiler -o build/test_runner.exe

# Run all tests
./build/test_runner.exe
```

## Test Framework

The test suite uses a simple custom test framework with automatic test registration.

### Writing Tests

```c
#include "test_harness.h"

TEST(test_name) {
    ASSERT_TRUE(condition);
    ASSERT_FALSE(condition);
    ASSERT_EQ(expected, actual);
    ASSERT_NE(not_expected, actual);
    ASSERT_STREQ(expected_str, actual_str);
    ASSERT_STRNE(not_expected_str, actual_str);
    ASSERT_NULL(ptr);
    ASSERT_NOT_NULL(ptr);
}
```

Tests are automatically registered using GCC constructor attributes and executed in registration order.

## Platform Support

Tests are designed to be cross-platform and use preprocessor directives to handle differences between:
- Windows (`_WIN32`)
- Unix-like systems (Linux, macOS)

Path separators and system commands are adjusted automatically based on the platform.

## Known Limitations

### Integration Tests
Integration tests that compile generated C code require `gcc` to be available in the system PATH. If `gcc` is not in PATH, these tests will fail with "command not found" errors.

To run only unit tests without integration tests, exclude the integration test files:
```bash
gcc tests/test_lexer.c tests/test_parser.c tests/test_typechecker.c tests/test_codegen.c tests/test_structs.c tests/test_harness.c tests/test_main.c compiler/lexer.c compiler/parser.c compiler/ast.c compiler/typechecker.c compiler/codegen.c -Icompiler -o build/test_runner_unit.exe
```

