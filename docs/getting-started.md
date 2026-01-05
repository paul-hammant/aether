# Getting Started with Aether

This guide will help you install and start using the Aether programming language.

## Installation

### Prerequisites

**All Platforms:**
- Git
- 1GB free disk space
- 2GB RAM minimum (4GB recommended for parallel builds)

**Linux/macOS:**
- GCC 9.0+ or Clang 10.0+
- Make
- pthread support (usually built-in)

**Windows:**
- MinGW-w64 GCC 11.0+ (MSVC not supported)
- PowerShell 5.1+ (comes with Windows 10+)
- Download MinGW: [mingw-w64.org](https://www.mingw-w64.org/) or via package manager

### Building from Source

```bash
git clone https://github.com/yourname/aether.git
cd aether

# Linux/macOS
make compiler

# Windows
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

Aether is built around the actor model. Here's a simple ping-pong example:

```aether
actor Pong {
    receive {
        "ping" => {
            print("Pong!")
            send msg.sender, "pong"
        }
    }
}

actor Ping {
    state {
        count: int
        pong: ActorRef[Pong]
    }
    
    receive {
        "start" => {
            count = 0
            send pong, "ping"
        }
        
        "pong" => {
            count = count + 1
            print("Ping " + count)
            
            if (count < 5) {
                send pong, "ping"
            }
        }
    }
}

main {
    let pong = spawn Pong
    let ping = spawn Ping
    
    send ping, "pong" -> pong
    send ping, "start"
}
```

## Module System

Import modules using the `import` statement:

```aether
import std.collections.HashMap
import std.log as Log

main {
    Log.info("Starting application")
    
    let map = HashMap.new()
    map.insert("key", "value")
    
    print(map.get("key"))
}
```

## Standard Library

Aether comes with a rich standard library:

### Collections
- **HashMap**: O(1) hash map with Robin Hood hashing
- **Set**: Set operations (union, intersection, difference)
- **Vector**: Dynamic array with amortized O(1) append
- **PriorityQueue**: Binary heap for priority-based scheduling

### Utilities
- **log**: Structured logging with levels
- **fs**: File system operations
- **net**: Networking utilities

## Pattern Matching

Aether supports powerful pattern matching:

```aether
match value {
    0 => print("Zero")
    1 => print("One")
    [h|t] => print("List with head: " + h)
    _ => print("Other")
}
```

## Next Steps

- Read the [Language Reference](language-reference.md)
- Explore [Standard Library Documentation](stdlib-reference.md)
- Check out [Example Programs](../examples/)
- Join the community on GitHub

## Performance Tips

1. Use pattern matching for cleaner code
2. Leverage the profiler to identify bottlenecks
3. Use appropriate collection types (HashMap for lookups, Vector for sequences)
4. Keep actors lightweight and focused
5. Use the import system to organize code

## Getting Help

- GitHub Issues: Report bugs and request features
- Documentation: Check docs/ folder
- Examples: See examples/ folder for working code

## Troubleshooting

### Build Failures

**"gcc: command not found" (Windows)**
- Ensure MinGW bin directory is in PATH
- Verify: `gcc --version`
- Add to PATH: `$env:PATH += ";C:\mingw64\bin"`

**"pthread.h: No such file or directory"**
- Install pthread library: `sudo apt-get install libpthread-stubs0-dev` (Linux)
- MinGW includes pthread by default

**Test failures**
- Run specific test category: `./build/test_runner --category=compiler`
- Check for port conflicts if network tests fail (port 8080)
- Ensure no antivirus blocking test executables

### Common Pitfalls

1. **Forgetting to rebuild after changes**: Run `make clean && make` or `.\build_compiler.ps1` after pulling updates
2. **Using MSVC on Windows**: Aether requires GCC. MSVC will not work.
3. **Missing -lpthread flag**: When compiling manually, always include `-lpthread`
4. **Running out of memory during parallel builds**: Reduce job count (`make -j4` instead of `-j8`)
5. **Permission denied on test_runner**: Ensure no previous test process is hanging (check Task Manager/htop)

### Platform-Specific Notes

**Windows MinGW:**
- Use PowerShell, not CMD
- Forward slashes in paths work: `./build/aetherc`
- Some ANSI colors may not render correctly in older terminals

**macOS:**
- May need to install command line tools: `xcode-select --install`
- Homebrew GCC recommended: `brew install gcc`

**Linux:**
- Requires kernel 4.14+ for full NUMA support
- AddressSanitizer requires `sudo apt-get install gcc-multilib` on some distributions
