# Aether Tools

Development tools for the Aether programming language.

## Directory Structure

```
tools/
├── ae.c                    # Unified CLI tool (recommended)
├── aether_repl.c           # Interactive REPL
├── apkg/                   # Package manager
│   ├── apkg.c/h           # Package manager implementation
│   ├── main.c             # Package manager entry point
│   └── toml_parser.c/h    # TOML configuration parser
├── profiler/               # Runtime profiler (see profiler/README.md)
├── benchmark_compare.py    # Benchmark result comparison
├── benchmark_runner.sh     # CI benchmark execution
├── check_regression.py     # Performance regression detection
└── test_apkg.sh           # Package manager tests
```

## Building Tools

All tools are built via the Makefile in the project root:

```bash
# Recommended: Build the unified CLI tool
make ae
./build/ae help

# Build individual tools
make repl       # Interactive REPL
make apkg       # Package manager
make profiler   # Profiler dashboard
```

## ae - Unified CLI Tool

The primary interface for working with Aether programs.

```bash
./build/ae run file.ae       # Compile and run
./build/ae build file.ae     # Build executable
./build/ae init myproject    # Create new project
./build/ae test              # Run project tests
./build/ae repl              # Start interactive REPL
./build/ae version           # Show version info
./build/ae help              # Show all commands
```

## REPL - Interactive Mode

Start an interactive session for quick experimentation:

```bash
make repl
./build/ae repl
```

### Requirements

readline library:
- macOS: `brew install readline`
- Ubuntu/Debian: `sudo apt-get install libreadline-dev`

### Commands

| Command | Shortcut | Description |
|---------|----------|-------------|
| `:help` | `:h` | Show help message |
| `:quit` | `:q` | Exit the REPL |
| `:clear` | `:c` | Clear the screen |
| `:multi` | `:m` | Enter multiline mode |
| `:reset` | `:r` | Reset the session |
| `:show` | `:s` | Show current session code |

### Examples

```aether
>>> 2 + 3
5

>>> x = 10
>>> y = 20
>>> x + y
30

>>> :multi
... actor Counter {
...     state count = 0
...     receive {
...         Increment() -> { count = count + 1 }
...     }
... }
...
OK
```

## Package Manager (apkg)

Manage Aether projects and dependencies:

```bash
make apkg
./build/apkg init myproject
./build/apkg build
./build/apkg test
```

## Profiler Dashboard

Web-based real-time profiler for monitoring runtime performance:

```bash
make profiler
./build/profiler_demo
# Open http://localhost:8080
```

See [profiler/README.md](profiler/README.md) for integration details.

## Benchmark Tools

Used by CI for performance tracking:

- `benchmark_runner.sh` - Executes benchmark suite
- `benchmark_compare.py` - Compares results between runs
- `check_regression.py` - Detects performance regressions

Run benchmarks:

```bash
make benchmark
```
