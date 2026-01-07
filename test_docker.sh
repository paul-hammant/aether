#!/bin/bash
# Test Docker Build and Makefile on Unix systems

set -e

echo "============================================"
echo "  Aether Docker & Makefile Test Suite"
echo "============================================"
echo ""

# Check Docker
echo "[1/5] Checking Docker installation..."
if ! command -v docker &> /dev/null; then
    echo "ERROR: Docker not found"
    exit 1
fi
echo "SUCCESS: Docker found"

# Check Docker daemon
echo ""
echo "[2/5] Checking Docker daemon..."
if ! docker info &> /dev/null; then
    echo "ERROR: Docker daemon not running"
    exit 1
fi
echo "SUCCESS: Docker daemon running"

# Build Docker image
echo ""
echo "[3/5] Building Docker image..."
docker build -t aether:test -f Dockerfile . --quiet
echo "SUCCESS: Docker image built"

# Test make
echo ""
echo "[4/5] Testing make in Docker..."
docker run --rm -v $(pwd):/aether -w /aether aether:test /bin/bash -c "make clean && make -j$(nproc) compiler"
echo "SUCCESS: Make build completed"

# Test compiler
echo ""
echo "[5/5] Testing compiler..."
docker run --rm -v $(pwd):/aether -w /aether aether:test /aether/build/aetherc --version
echo "SUCCESS: Compiler works"

echo ""
echo "============================================"
echo "  All Tests Passed!"
echo "============================================"
