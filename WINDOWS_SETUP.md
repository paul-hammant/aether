# Windows Development Setup

This guide covers setting up the Aether development environment on Windows.

## Option 1: Native Windows Build (Recommended)

### Prerequisites

1. **MinGW-w64 with GCC**
   - Download from: https://www.mingw-w64.org/
   - Or install via WinGet: `winget install -e --id mingw-w64.mingw-w64`
   - Make sure `gcc.exe` is in your PATH

2. **Optional: Make for Windows**
   ```powershell
   # Using Chocolatey
   choco install make
   
   # Using WinGet (via MSYS2)
   winget install -e --id MSYS2.MSYS2
   # Then in MSYS2: pacman -S make
   
   # Using Scoop
   scoop install make
   ```

### Building without Make

Use the provided PowerShell build script (recommended for Windows):

```powershell
# Fast build (single compilation unit)
.\build.ps1 -Fast

# Parallel build (2-4x faster)
.\build.ps1 -Parallel

# Release build with optimizations
.\build.ps1 -Release

# Build everything
.\build.ps1 -All

# Run tests
.\build.ps1 -Test

# Clean rebuild
.\build.ps1 -Clean -Fast
```

### Building with Make

If you have `make` installed (via MSYS2, Git Bash, or Chocolatey):

```bash
# In Git Bash or MSYS2
make compiler
make test
make clean
```

Or use mingw32-make if available:
```powershell
mingw32-make compiler
mingw32-make test
```

## Option 2: Docker (Cross-Platform)

Docker provides a consistent Linux environment on Windows. This is the **best option** for testing cross-platform compatibility.

### Prerequisites

1. **Docker Desktop for Windows**
   - Download: https://www.docker.com/products/docker-desktop/
   - Or via WinGet: `winget install Docker.DockerDesktop`

2. **Enable WSL 2** (recommended)
   ```powershell
   wsl --install
   wsl --set-default-version 2
   ```

### Docker Quick Start

```powershell
# Build the Docker image
docker build -t aether:latest .

# Run interactive shell in container
docker run -it -v ${PWD}:/aether aether:latest /bin/bash

# Inside container, you can use make:
make clean
make compiler
make test

# Or run from PowerShell:
docker run -v ${PWD}:/aether aether:latest make compiler
docker run -v ${PWD}:/aether aether:latest make test
```

### Docker Compose (Advanced)

For a full development environment:

```powershell
# Start development environment
docker-compose up -d aether-dev

# Enter the container
docker-compose exec aether-dev /bin/bash

# Run tests
docker-compose run aether-test

# Build everything
docker-compose run aether-build make all

# Stop all containers
docker-compose down
```

### Inside Docker Container

```bash
# Full make support
make clean
make compiler          # Build compiler (5-6 seconds)
make test             # Run all tests
make lsp              # Build LSP server
make profiler         # Build profiler
make all              # Build everything

# Examples
./build/aetherc examples/basic/hello_world.ae output.c
gcc output.c -o hello -pthread
./hello
```

## Option 3: WSL 2 (Windows Subsystem for Linux)

Best of both worlds - native Linux environment on Windows.

### Setup

```powershell
# Install WSL 2
wsl --install Ubuntu-22.04

# Enter WSL
wsl

# Inside WSL:
sudo apt update
sudo apt install -y gcc make git valgrind

# Navigate to your project (mounted at /mnt/d/)
cd /mnt/d/Git/aether

# Build using make
make compiler
make test
```

## Comparison

| Method | Pros | Cons | Best For |
|--------|------|------|----------|
| **PowerShell Script** | Native, fast, no dependencies | Windows-only | Daily Windows development |
| **Docker** | Reproducible, cross-platform, full make support | Slight overhead, requires Docker | Testing, CI/CD, consistency |
| **WSL 2** | Linux environment, fast, integrated | Requires WSL setup | Full Linux tooling |
| **Native Make** | Standard build tool | Requires installation | Unix-like workflows |

## Testing the Build

### Quick Verification

```powershell
# Windows native
.\build.ps1 -Fast
.\build\aetherc.exe --version

# Docker
docker run -v ${PWD}:/aether aether:latest /aether/build/aetherc --version

# WSL
wsl ./build/aetherc --version
```

### Full Test Suite

```powershell
# Windows
.\build.ps1 -Test

# Docker
docker run -v ${PWD}:/aether aether:latest make test

# WSL
wsl make test
```

## Makefile Compatibility Notes

The Makefile includes Windows detection:

```makefile
ifeq ($(OS),Windows_NT)
    EXE_EXT := .exe
    LDFLAGS += -lws2_32
    MKDIR := if not exist build mkdir build
endif
```

This means the Makefile **will work** on Windows if you have:
- `make` or `mingw32-make` installed
- MinGW-w64 GCC in your PATH
- Git Bash, MSYS2, or WSL

## Recommended Setup for Different Use Cases

### For Contributors
- Primary: Docker (consistent environment)
- Secondary: Native Windows with PowerShell scripts

### For Daily Development
- Windows users: `build.ps1` with `-Parallel` flag
- Linux/Mac users: `make` with `-j$(nproc)`

### For CI/CD
- Docker (already configured in `Dockerfile`)

## Troubleshooting

### "make: command not found"
Solution: Use `.\build.ps1` instead, or install make via Chocolatey/MSYS2

### "gcc: command not found"
Solution: Install MinGW-w64 and add to PATH

### Docker build fails
Solution: Ensure Docker Desktop is running and WSL 2 is enabled

### Permission errors in Docker
Solution: Run Docker as administrator or fix WSL 2 file permissions

## Next Steps

1. Choose your preferred method above
2. Build the compiler: `.\build.ps1 -Fast` or `make compiler`
3. Run tests: `.\build.ps1 -Test` or `make test`
4. Try examples: `.\build\aetherc.exe examples/basic/hello_world.ae output.c`

## Additional Resources

- [Docker Documentation](https://docs.docker.com/)
- [WSL 2 Guide](https://learn.microsoft.com/en-us/windows/wsl/)
- [MinGW-w64](https://www.mingw-w64.org/)
- [MSYS2](https://www.msys2.org/)
