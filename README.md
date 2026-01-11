# Aether Programming Language

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)]()

An experimental actor-based programming language with gradual typing. Aether explores actor concurrency patterns by compiling to C, making it a research platform for understanding concurrent systems design.

**Project Status:** Early development and experimental. Not recommended for production use.

## Overview

Aether is a compiled language exploring Erlang-style actor concurrency in a systems programming context. The compiler generates C code, providing portability across platforms.

**Design Goals:**
- Explore actor-based concurrency patterns
- Gradual type system for prototyping
- Learn from existing C libraries through direct interop
- Generate readable C code for understanding compilation

## Runtime Implementation

The runtime implements concurrent programming techniques for actor-based message passing:

**Scheduler:**
- Partitioned multi-core scheduler (actors bound to cores)
- Lock-free SPSC queues for cross-core messaging
- Message coalescing for 15x throughput improvement
- Progressive backoff (spin, pause, yield) for power efficiency

**Synchronization:**
- Optimized spinlocks with PAUSE instruction (3x faster)
- Cache line alignment to prevent false sharing
- Atomic operations with explicit memory ordering

**Message Passing:**
- Lock-free mailbox implementation (1.8x improvement)
- Zero atomic contention in actor processing hot path
- Backpressure handling for queue overflow

**Performance:**
- 625,000 cross-core messages/sec
- 6.45M messages/sec on 4 cores
- Sub-millisecond message latency

See [docs/runtime-optimizations.md](docs/runtime-optimizations.md) for implementation details and benchmarking methodology.

## Features

### Concurrency
- Actor-based message passing
- Multi-core scheduler implementation
- Lock-free mailbox experiments

### Type System
- Static type checking
- Type inference for local variables
- Type checking pass in compiler

### Memory Management
- Arena allocation for actor lifetimes
- Memory pool experiments
- Manual memory management (no GC)

### C Interoperability
- Compiles to C99 code
- Can call C functions
- Uses C compiler toolchain

## Quick Start

### Prerequisites

**Required:**
- GCC 11+ or Clang 14+
- Make (or mingw32-make on Windows)
- Git

**Optional:**
- Docker Desktop (recommended for consistent environment)

### Installation

```bash
git clone https://github.com/nicolasmd87/aether.git
cd aether

# Build compiler and CLI tool
make compiler
make ae

# Test it works
./build/ae version
# Aether 0.4.0
```

**Windows:** Use `mingw32-make` instead of `make`.

### Your First Program

Create `hello.ae`:

```aether
main() {
    print("Hello from Aether!\\n");
    print("The answer is: %d\\n", 42);
}
```

Run it:

```bash
./build/ae run hello.ae
```

That's it! No configuration, no build files needed.

**Docker (Recommended for Windows):**

```bash
# Build container
docker build -t aether:latest -f docker/Dockerfile .

# Start development shell
docker run -it -v $(pwd):/aether aether:latest

# Inside container
make compiler
make test
```

See [docker/README.md](docker/README.md) for detailed setup.

### Running Your First Program

Create `hello.ae`:

```aether
main() {
    print("Hello from Aether!\n");
    print("The answer is: %d\n", 42);
}
```

**Go-style commands (recommended):**

```bash
# Run directly
./build/ae run hello.ae

# Or build to executable
./build/ae build hello.ae -o hello
./build/hello
```

**Using Make:**

```bash
# Run
make run FILE=hello.ae

# Build
make compile FILE=hello.ae OUTPUT=hello
```

**Manual (advanced):**

```bash
# 1. Compile .ae to .c
./build/aetherc hello.ae hello.c

# 2. Compile C to executable
gcc hello.c -o hello

# 3. Run
./hello
```

### Common Commands

**Using ae (Go-style):**

```bash
./build/ae run hello.ae          # Run a program
./build/ae build app.ae -o myapp # Build executable
./build/ae compile lib.ae        # Compile to C only
./build/ae test                  # Run tests
./build/ae help                  # Show help
```

**Using Make:**

```bash
make compiler                    # Build compiler
make ae                          # Build ae CLI tool
make run FILE=hello.ae           # Run a program
make compile FILE=app.ae         # Build executable
make test                        # Run tests
make -j8                         # Parallel build
make help                        # Show all commands
```

**Windows:** Use `mingw32-make` instead of `make` and `.\build\ae.exe` instead of `./build/ae`.

### Adding `ae` to PATH

**Linux/macOS:**
```bash
# After building
sudo cp build/ae /usr/local/bin/
# Or add to ~/.bashrc:
export PATH="$PATH:/path/to/aether/build"

# Now use anywhere:
ae run myapp.ae
```

**Windows:**
```powershell
# Add to PATH via System Properties > Environment Variables
# Or use full path:
D:\Git\aether\build\ae.exe run myapp.ae
```

## Build System Philosophy

Aether provides two interfaces:

**1. `ae` command (Go-style) - Recommended**

```bash
./build/ae run file.ae           # Like: go run, cargo run, zig run
./build/ae build file.ae         # Like: go build, cargo build, zig build
```

Simple, fast, no configuration needed.

**2. Make (traditional) - For advanced use**

```bash
make compiler                    # Build the compiler
make run FILE=file.ae           # Run with Make
make test                       # Run tests
```

**Comparison with other languages:**

| Language | Tool | Run Command | Build Command |
|----------|------|-------------|---------------|
| **Aether** | ae | `ae run app.ae` | `ae build app.ae` |
| Go | go | `go run app.go` | `go build app.go` |
| Rust | cargo | `cargo run` | `cargo build` |
| Zig | zig | `zig run app.zig` | `zig build` |
| C/C++ | Make | Manual | `make` |

**Why both?**
- `ae` command: Simple, fast for daily development
- Make: Full control, parallel builds, CI/CD integration

## Project Status

**Aether is experimental software.** It's a research project exploring actor-based concurrency and compiler design. Not recommended for production use.

**Current capabilities:**
- Basic compiler (lexer, parser, type checker, code generator)
- Actor runtime with message passing
- Multi-core scheduler
- Standard library (collections, I/O basics)

**Limitations:**
- No distribution (single-node only)
- No hot code reloading
- Limited error messages
- Manual memory management
- Small ecosystem
- Breaking changes expected

**Better alternatives for production:**
- Erlang/Elixir (distributed actors, proven at scale)
- Go (goroutines, mature ecosystem)
- Rust (memory safety, systems programming)
- Pony (actor-based with reference capabilities)

See [docs/competitive-analysis.md](docs/competitive-analysis.md) for detailed comparison.

```
aether/
├── compiler/           # Aether compiler (lexer, parser, codegen)
│   ├── lexer.c/h      # Tokenization
│   ├── parser.c/h     # AST generation
│   ├── typechecker.c/h # Type inference and checking
│   ├── codegen.c/h    # C code generation
│   └── aetherc.c      # Compiler entry point
├── runtime/           # Runtime system
│   ├── actors/        # Actor implementation and scheduling
│   ├── memory/        # Arena allocators and memory pools
│   ├── scheduler/     # Multi-core work-stealing scheduler
│   └── utils/         # CPU detection, SIMD, tracing
├── std/               # Standard library
│   ├── collections/   # HashMap, Vector, List
│   ├── io/           # File I/O, streams
│   ├── net/          # TCP/UDP networking
│   └── json/         # JSON parser
├── tests/            # Test suite
│   ├── compiler/     # Compiler unit tests
│   ├── runtime/      # Runtime tests
│   └── integration/  # End-to-end tests
├── examples/         # Example programs
│   ├── basic/        # Hello world, simple actors (.ae files)
│   ├── language-features/ # Type system, syntax (.ae files)
│   ├── benchmarks/   # Aether benchmark examples (.ae files)
│   └── real-world/   # Web servers, chat apps (.ae files)
├── benchmarks/       # C performance benchmarks
├── runtime/
│   ├── actors/       # Actor runtime implementations
│   ├── memory/       # Memory management  
│   └── examples/     # C API usage examples
├── experiments/      # Performance optimization research
│   └── concurrency/  # Historical explorations (01-07)
├── docs/            # Documentation
│   ├── getting-started.md
│   ├── language-reference.md
│   ├── tutorial.md
│   └── architecture.md
└── docker/          # Docker configuration
```

## Language Example

```aether
// Distributed counter with actor supervision
actor Counter {
    var count = 0
    
    receive {
        Increment => count += 1
        
        Decrement => count -= 1
        
        GetCount(reply) => {
            reply.send(count)
        }
        
        Reset => count = 0
    }
}

actor Supervisor {
    var counters: [ActorRef<Counter>] = []
    
    init() {
        // Spawn 4 counter actors
        for i in 0..4 {
            counters.push(spawn Counter)
        }
    }
    
    receive {
        IncrementAll => {
            for counter in counters {
                counter ! Increment
            }
        }
        
        GetTotal(reply) => {
            var total = 0
            for counter in counters {
                let count = counter !? GetCount
                total += count
            }
            reply.send(total)
        }
    }
}

// Main program
let supervisor = spawn Supervisor
supervisor ! IncrementAll
let total = supervisor !? GetTotal
println("Total count: {total}")
```

## Advanced Features

### Runtime Configuration

Configure runtime optimizations at startup:

```c
#include "runtime/aether_runtime.h"

int main() {
    // Auto-detect CPU features and enable optimizations
    aether_runtime_init(4, AETHER_FLAG_AUTO_DETECT);
    
    // Or manually configure
    aether_runtime_init(4, 
        AETHER_FLAG_LOCKFREE_MAILBOX |
        AETHER_FLAG_LOCKFREE_POOLS |
        AETHER_FLAG_ENABLE_MWAIT |
        AETHER_FLAG_VERBOSE
    );
    
    // Your actor system runs here
    
    return 0;
}
```

Available flags:
- `AETHER_FLAG_AUTO_DETECT` - Detect CPU features and enable optimizations
- `AETHER_FLAG_LOCKFREE_MAILBOX` - Use lock-free SPSC mailboxes
- `AETHER_FLAG_LOCKFREE_POOLS` - Thread-local message pools
- `AETHER_FLAG_ENABLE_SIMD` - AVX2 vectorization
- `AETHER_FLAG_ENABLE_MWAIT` - MWAIT-based idle (x86 only)
- `AETHER_FLAG_VERBOSE` - Print runtime configuration

### Gradual Typing

Start with dynamic typing, add types incrementally:

```aether
// Fully dynamic
actor Database {
    var data = {}
    
    receive {
        Store(key, value) => data[key] = value
        Get(key, reply) => reply.send(data[key])
    }
}

// Add types for safety
actor Database {
    var data: HashMap<String, Int> = {}
    
    receive {
        Store(key: String, value: Int) => {
            data[key] = value
        }
        
        Get(key: String, reply: ActorRef<Int>) => {
            reply.send(data[key])
        }
    }
}
```

### C Interoperability

Call C functions directly:

```aether
// Import C function
extern fn sqrt(x: f64) -> f64

actor MathService {
    receive {
        CalculateSqrt(x, reply) => {
            let result = sqrt(x as f64)
            reply.send(result)
        }
    }
}
```

## Documentation

- [Getting Started Guide](docs/getting-started.md) - Installation and first steps
- [Language Tutorial](docs/tutorial.md) - Learn Aether syntax and concepts
- [Language Reference](docs/language-reference.md) - Complete language specification
- [Architecture Overview](docs/architecture.md) - Runtime and compiler design
- [Type System Guide](docs/gradual-typing.md) - Gradual typing and inference
- [Standard Library](docs/stdlib-reference.md) - Collections, I/O, networking
- [Docker Setup](docker/README.md) - Container development environment

## Development

### Running Tests

```bash
# All tests
make test

# Specific test suite
./build/test_harness compiler    # Compiler tests
./build/test_harness runtime     # Runtime tests
./build/test_harness integration # Integration tests
```

### Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Code of Conduct

This project follows the [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you agree to uphold this code.

## Roadmap

### Current Status (v0.1-alpha)
- Basic compiler pipeline functional
- Actor runtime with local message passing
- Type checker with gradual typing
- Standard library (collections, I/O basics)

### Development Priorities
- Stabilize core language features
- Improve error messages
- Expand test coverage
- Documentation completeness

### Exploratory Features
- Package management concepts
- Editor integration experiments
- Runtime optimization research

### Not Planned (Requires Significant Resources)
- Production hardening
- Enterprise support
- Distributed actors
- Hot code reloading

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

This project explores ideas from:
- Erlang/OTP (actor model)
- Go (pragmatic concurrency)
- Rust (systems programming practices)
- Pony (actor-based type safety)

Aether is a learning project and research platform, not a competitor to these established languages.

## Contact

**Note:** This is an experimental project for learning and research. For production actor systems, consider Erlang, Elixir, or Pony.
---

**Built with performance and simplicity in mind.**

