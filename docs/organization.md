# Folder Organization

The project has been reorganized for better maintainability:

## Structure

```
aether/
├── docker/                    # Docker configuration
│   ├── Dockerfile            # Production build environment
│   ├── Dockerfile.dev        # Development environment
│   ├── docker-compose.yml    # Multi-service setup
│   └── README.md             # Docker quick start
│
├── scripts/                   # Test scripts
│   └── test/
│       └── test_docker.sh    # Docker test (Linux/Mac)
│
├── docs/                      # Documentation
│   ├── setup/
│   │   ├── DOCKER.md         # Full Docker guide
│   │   └── WINDOWS_SETUP.md  # Windows setup guide
│   └── ... (other docs)
│
├── compiler/                  # Compiler source (organized)
│   ├── frontend/             # Lexer, parser, tokens
│   ├── backend/              # Codegen, optimizer
│   ├── analysis/             # Type checker, inference
│   └── ... (support files)
│
├── runtime/                   # Runtime source (organized)
│   ├── actors/               # Actor system
│   ├── scheduler/            # Scheduler & queues
│   ├── memory/               # Memory management
│   ├── simd/                 # SIMD operations
│   └── utils/                # Utilities
│
├── aether.ps1                # Main CLI (Windows only)
├── Makefile                  # Build system (all platforms)
├── README.md                 # Main documentation
└── ... (source code)
```

## Building

Use the Makefile for all builds:

```bash
# Build compiler
make compiler

# Clean and rebuild
make clean
make compiler

# Run tests
make test
```

On Windows with MinGW, use mingw32-make:

```powershell
mingw32-make compiler
mingw32-make test
```

## Docker

```bash
# Build
docker build -t aether:latest -f docker/Dockerfile .

# Run
docker-compose -f docker/docker-compose.yml up -d aether-dev

# Test
./scripts/test/test_docker.sh     # Linux/Mac
```

## Documentation

- Quick start: [README.md](../README.md)
- Architecture: [architecture.md](architecture.md)
- Actor system: [actor-concurrency.md](actor-concurrency.md)
- Performance: [../PERFORMANCE.md](../PERFORMANCE.md)
- NUMA support: [numa-support.md](numa-support.md)
- Windows setup: [setup/WINDOWS_SETUP.md](setup/WINDOWS_SETUP.md)
- Docker guide: [setup/DOCKER.md](setup/DOCKER.md)

## Benefits

- Clean root directory - No clutter
- Logical grouping - Easy to find files
- Scalable - Can add more without mess
- Industry standard - Matches LLVM, Rust, Go
- Backwards compatible - All commands still work
