# Aether Programming Language

A high-performance actor-based language with automatic memory management and strong type inference.

## Features

- **Actor-based concurrency**: Lightweight actors with message passing
- **Type inference**: Optional type annotations with full inference
- **Memory safety**: Automatic memory management without GC pauses
- **High performance**: 2.3B messages/sec on commodity hardware
- **Cross-platform**: Windows, Linux, and macOS support

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

## Building

```bash
# Build from source
.\build.ps1         # Windows
make                # Linux/macOS

# Run tests
.\aether.ps1 test   # Windows  
make test           # Linux/macOS
```

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