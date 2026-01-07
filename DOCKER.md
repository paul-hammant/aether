# Docker Quick Reference

## Building Images

```bash
# Build production image
docker build -t aether:latest .

# Build development image
docker build -t aether-dev:latest -f Dockerfile.dev .

# Build with specific tag
docker build -t aether:0.4.0 .
```

## Running Containers

### Interactive Shell

```bash
# Production environment
docker run -it -v $(pwd):/aether aether:latest /bin/bash

# Development environment
docker run -it -v $(pwd):/aether aether-dev:latest /bin/bash

# Windows PowerShell
docker run -it -v ${PWD}:/aether aether:latest /bin/bash
```

### Build Commands

```bash
# Build compiler using make
docker run --rm -v $(pwd):/aether -w /aether aether:latest make compiler

# Build and run tests
docker run --rm -v $(pwd):/aether -w /aether aether:latest make test

# Clean and rebuild
docker run --rm -v $(pwd):/aether -w /aether aether:latest bash -c "make clean && make -j4 compiler"

# Build everything
docker run --rm -v $(pwd):/aether -w /aether aether:latest make all
```

### Compile Aether Programs

```bash
# Compile a program
docker run --rm -v $(pwd):/work aether:latest aetherc /work/examples/basic/hello_world.ae /work/output.c

# Compile and build
docker run --rm -v $(pwd):/work aether:latest bash -c "
  aetherc /work/examples/basic/hello_world.ae /tmp/output.c && 
  gcc /tmp/output.c -o /work/hello -pthread && 
  echo 'Build successful'
"
```

## Docker Compose

### Start Development Environment

```bash
# Start all services
docker-compose up -d

# Start specific service
docker-compose up -d aether-dev

# View logs
docker-compose logs -f aether-dev
```

### Execute Commands

```bash
# Enter running container
docker-compose exec aether-dev /bin/bash

# Run single command
docker-compose exec aether-dev make compiler

# Run tests
docker-compose run aether-test

# Build everything
docker-compose exec aether-dev make all
```

### Stop and Clean

```bash
# Stop all services
docker-compose down

# Stop and remove volumes
docker-compose down -v

# Rebuild images
docker-compose build --no-cache
```

## Windows-Specific Commands

### PowerShell

```powershell
# Build
docker build -t aether:latest .

# Run with volume mounting
docker run -it -v ${PWD}:/aether aether:latest /bin/bash

# Test
.\test_docker.ps1

# Docker Compose
docker-compose up -d
docker-compose exec aether-dev /bin/bash
```

### CMD

```cmd
# Build
docker build -t aether:latest .

# Run
docker run -it -v %cd%:/aether aether:latest /bin/bash
```

## Useful Workflows

### Fresh Build and Test

```bash
# Clean everything and rebuild
docker run --rm -v $(pwd):/aether -w /aether aether:latest bash -c "
  make clean && 
  make -j4 compiler && 
  make test
"
```

### Development Cycle

```bash
# 1. Start dev container
docker-compose up -d aether-dev

# 2. Enter container
docker-compose exec aether-dev /bin/bash

# 3. Inside container, work normally
make clean
make compiler
./build/aetherc examples/basic/hello_world.ae output.c
gcc output.c -o hello -pthread
./hello

# 4. Exit and stop
exit
docker-compose down
```

### CI/CD Pipeline

```bash
# Single command for CI
docker run --rm -v $(pwd):/aether -w /aether aether:latest bash -c "
  make clean &&
  make -j\$(nproc) compiler &&
  make test &&
  echo 'All checks passed'
"
```

### Memory Testing with Valgrind

```bash
docker run --rm -v $(pwd):/aether -w /aether aether:latest bash -c "
  make compiler &&
  valgrind --leak-check=full ./build/aetherc --version
"
```

## Troubleshooting

### Permission Issues

```bash
# Run with current user ID
docker run --rm --user $(id -u):$(id -g) -v $(pwd):/aether aether:latest make compiler
```

### File Changes Not Reflected

```bash
# Rebuild image
docker build --no-cache -t aether:latest .

# Or use bind mount (already done with -v flag)
```

### Out of Disk Space

```bash
# Clean up Docker
docker system prune -a

# Remove old images
docker image prune -a

# Remove all containers
docker container prune
```

### Container Won't Start

```bash
# Check logs
docker logs <container_id>

# Check Docker daemon
docker info

# Restart Docker Desktop (Windows)
```

## Performance Tips

### Use ccache

```bash
# Already configured in Dockerfile.dev
docker run -v $(pwd):/aether -v ccache:/root/.ccache aether-dev:latest make compiler
```

### Multi-core Builds

```bash
# Linux/Mac
docker run --rm -v $(pwd):/aether -w /aether aether:latest make -j$(nproc) compiler

# Windows (PowerShell)
docker run --rm -v ${PWD}:/aether -w /aether aether:latest make -j4 compiler
```

### Layer Caching

```dockerfile
# Put frequently changing files last in Dockerfile
COPY Makefile .
COPY build.ps1 .
COPY compiler/ compiler/
COPY runtime/ runtime/
# ... rest of files
```

## Example: Full Workflow

```bash
# 1. Clone repository
git clone https://github.com/youruser/aether.git
cd aether

# 2. Build Docker image
docker build -t aether:latest .

# 3. Test the build
./test_docker.sh   # Linux/Mac
# or
.\test_docker.ps1  # Windows

# 4. Start development
docker-compose up -d aether-dev
docker-compose exec aether-dev /bin/bash

# 5. Inside container
make compiler
make test

# 6. Compile example
./build/aetherc examples/basic/hello_world.ae output.c
gcc output.c -o hello -pthread
./hello

# 7. Exit
exit
docker-compose down
```

## Additional Resources

- [Docker Documentation](https://docs.docker.com/)
- [Docker Compose](https://docs.docker.com/compose/)
- [Aether WINDOWS_SETUP.md](WINDOWS_SETUP.md)
- [Dockerfile Best Practices](https://docs.docker.com/develop/develop-images/dockerfile_best-practices/)
