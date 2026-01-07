# Aether Programming Language

A high-performance actor-based systems programming language with lightweight concurrency, strong type inference, and zero-cost C interoperability.

## Key Features

- **Actor Concurrency**: Lightweight actors (128 bytes each) with message passing at 732M ops/sec
- **Zero-Sharing Architecture**: Partitioned scheduler with no lock contention in hot paths
- **Performance**: 291M messages/sec on 8 cores with near-linear scaling
- **Type System**: Hindley-Milner inference with optional annotations
- **Memory Safety**: Arena-based allocation with automatic lifetime management
- **C Interoperability**: Compiles to native C for zero-cost FFI and embedding
- **Cross-Platform**: Windows, Linux, and macOS support

## Quick Start

### Installation

```bash
git clone https://github.com/nicolasmd87/aether.git
cd aether

# Windows
.\build.ps1

# Linux/macOS
make
```

### Using Aether

```powershell
# Start interactive REPL (Windows)
.\aether.ps1

# Run a program (Windows)
.\aether.ps1 run examples/hello.ae

# Run tests (Windows)
.\aether.ps1 test

# Linux/macOS
make repl              # Start REPL
make run FILE=file.ae  # Run a program
make test              # Run tests
```

### Commands

**Windows:**
- `.\aether.ps1` - Start REPL (default)
- `.\aether.ps1 run <file>` - Compile and run
- `.\aether.ps1 compile <file>` - Compile to C
- `.\aether.ps1 test` - Run all tests
- `.\aether.ps1 help` - Show all commands

**Linux/macOS:**
- `make repl` - Start REPL
- `make run FILE=<file>` - Run a file
- `make test` - Run tests
- `make` - Build everything

## Building from Source

### Prerequisites

- **GCC** 11+ or Clang 14+
- **Make** (optional, can use `build.ps1` on Windows)
- **Git**

### Platform-Specific Setup

**Windows:**
- Install MinGW-w64: https://www.mingw-w64.org/
- Use `build.ps1` PowerShell script (no make required)
- See [WINDOWS_SETUP.md](WINDOWS_SETUP.md) for detailed guide

**Linux/macOS:**
- Install GCC: `sudo apt install gcc make` (Ubuntu) or `brew install gcc` (macOS)
- Use Makefile for builds

**Docker (All Platforms):**
- Install Docker Desktop
- See [DOCKER.md](DOCKER.md) for complete guide
- Provides consistent Linux environment on any OS

### Quick Build (Development)

```bash
# Windows - PowerShell script (Recommended)
.\build.ps1 -Fast              # Fast build
.\build.ps1 -Parallel          # Parallel build (2-4x faster)

# Linux/macOS - Makefile
make compiler                  # Build compiler
make -j$(nproc) compiler       # Parallel build

# Docker (Any OS)
docker build -t aether:latest .
docker run -it -v $(pwd):/aether aether:latest make compiler
```

### Docker Quick Start

Docker provides the easiest cross-platform development environment:

```powershell
# Windows PowerShell
docker build -t aether:latest .
docker run -it -v ${PWD}:/aether aether:latest /bin/bash

# Inside container
make compiler
make test
./build/aetherc examples/basic/hello_world.ae output.c
```

See [DOCKER.md](DOCKER.md) for:
- Docker Compose workflows
- Development environment setup
- CI/CD integration
- Volume mounting and file sharing

### Optimized Build (Production)

For production deployments, use aggressive optimization flags:

```bash
gcc compiler/*.c runtime/aether_message_registry.c runtime/aether_actor_thread.c \
    -I runtime -o aetherc.exe -O3 -march=native -Wall
```

**Optimization Flags Explained:**
- `-O3`: Aggressive optimizations (inlining, loop unrolling, vectorization)
- `-march=native`: Use CPU-specific instructions (AVX2, SSE4.2, etc.)
- `-flto`: Link-time optimization for whole-program analysis (optional)

### Profile-Guided Optimization (PGO)

PGO is an **industry-standard** technique used by major projects (Chrome, Firefox, LLVM, PostgreSQL) that provides 10-20% additional performance by optimizing based on actual runtime behavior.

**Why PGO?**
- Optimizes branch predictions based on real execution patterns
- Improves function inlining decisions using call frequency data
- Places hot code together to improve instruction cache utilization
- Used in production builds of all major browsers and compilers

**3-Stage PGO Build:**

```bash
# Windows
.\tools\build_pgo.ps1

# Linux/macOS
./tools/build_pgo.sh
```

This runs:
1. Build with instrumentation (`-fprofile-generate`)
2. Run benchmarks to collect profile data
3. Rebuild using profile data (`-fprofile-use`)

**When to use PGO:**
- Production deployments requiring maximum performance
- After implementing new hot-path features
- When benchmarks show performance regressions

**When not needed:**
- Development builds (adds ~3x build time)
- Debugging or profiling
- Rapid iteration during development

## Quick Example

```aether
actor Counter {
    var count = 0
    
    receive {
        Increment => count += 1
        GetCount(reply) => reply.send(count)
    }
}

let counter = spawn Counter
counter ! Increment
```

See [docs/tutorial.md](docs/tutorial.md) for a complete guide.