# Getting Started with Aether

This guide covers installation and basic usage of the Aether programming language.

## Installation

### Prerequisites

**All Platforms:**
- Git
- GCC 9.0+ or Clang 10.0+
- Make
- pthread support (usually built-in)

**Windows:**
- MinGW-w64 GCC 11.0+ (MSVC is not supported)
- PowerShell 5.1+

**macOS:**
- Xcode command line tools: `xcode-select --install`
- Homebrew GCC recommended: `brew install gcc`

**Linux:**
- `sudo apt-get install build-essential` (Debian/Ubuntu)

### Building from Source

```bash
git clone https://github.com/yourname/aether.git
cd aether

# Linux/macOS
make compiler

# Windows (MinGW)
./build_compiler.ps1
```

## Your First Aether Program

Create a file named `hello.ae`:

```aether
main {
    print("Hello, Aether!")
}
```

Compile and run:

```bash
./build/aetherc run hello.ae
```

## Actor-Based Programming

Aether is built around the actor model. Here is a simple counter example:

```aether
actor counter {
    state int count = 0;

    receive(msg) {
        count++;
        print(count);
    }
}

main() {
    c = spawn_counter();
    send_counter(c, 1, 0);
}
```

Actors are lightweight concurrent entities that communicate through asynchronous messages. Each actor has private state and a mailbox for incoming messages. The runtime distributes actors across available CPU cores automatically.

## Module System

Import modules using the `import` statement:

```aether
import std.collections.HashMap
import std.log as Log

main {
    Log.info("Starting application")

    map = HashMap.new()
    map.insert("key", "value")

    print(map.get("key"))
}
```

## Standard Library

Aether includes a standard library with the following modules:

### Collections
- **HashMap**: Hash map with Robin Hood hashing
- **Set**: Set operations (union, intersection, difference)
- **Vector**: Dynamic array with amortized O(1) append
- **PriorityQueue**: Binary heap for priority-based scheduling

### Utilities
- **log**: Structured logging with levels
- **fs**: File system operations
- **net**: Networking utilities

See [stdlib-reference.md](stdlib-reference.md) for the full API reference.

## Pattern Matching

Aether supports pattern matching:

```aether
match value {
    0 => print("Zero")
    1 => print("One")
    [h|t] => print("List with head: " + h)
    _ => print("Other")
}
```

## Next Steps

- Read the [Tutorial](tutorial.md) for a guided introduction
- Explore [Standard Library Documentation](stdlib-reference.md)
- See [Architecture](architecture.md) for compiler and runtime internals
- See [Runtime Optimizations](runtime-optimizations.md) for performance details

## Troubleshooting

### Build Failures

**"gcc: command not found" (Windows)**
- Ensure MinGW bin directory is in PATH
- Verify: `gcc --version`

**"pthread.h: No such file or directory"**
- Linux: `sudo apt-get install libpthread-stubs0-dev`
- MinGW includes pthread by default

**Test failures**
- Run specific test category: `./build/test_runner --category=compiler`
- Check for port conflicts if network tests fail (port 8080)

### Common Pitfalls

1. Forgetting to rebuild after changes: run `make clean && make`
2. Using MSVC on Windows: Aether requires GCC (MinGW)
3. Missing `-lpthread` flag when compiling manually
4. Actor structs missing the `migrate_to` field (causes struct layout mismatch)

### Platform-Specific Notes

**macOS:**
- May need `xcode-select --install` for command line tools
- Homebrew GCC recommended: `brew install gcc`

**Linux:**
- Kernel 4.14+ recommended for full NUMA support
- AddressSanitizer may require `gcc-multilib` on some distributions

**Windows:**
- Use PowerShell, not CMD
- Forward slashes in paths work: `./build/aetherc`
