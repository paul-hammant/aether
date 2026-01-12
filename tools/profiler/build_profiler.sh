#!/usr/bin/env bash
# Build Aether Profiler (Unix/Linux/macOS)

echo "========================================="
echo "Building Aether Profiler Dashboard"
echo "========================================="
echo ""

# Check for gcc
if ! command -v gcc &> /dev/null; then
    echo "ERROR: gcc not found in PATH"
    exit 1
fi

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Create build directory
mkdir -p ../../build

PROFILER_SRC=(
    "profiler_server.c"
    "profiler_demo.c"
    "../../runtime/memory/aether_arena.c"
    "../../runtime/memory/aether_pool.c"
    "../../runtime/memory/aether_memory_stats.c"
    "../../runtime/utils/aether_tracing.c"
)

CFLAGS="-O2 -I../../runtime -I../../runtime/memory -I../../runtime/utils -I../../std -I../../std/net -Wall -Wextra -Wno-unused-parameter -DAETHER_PROFILING"
LDFLAGS="-pthread"

echo "Compiling profiler demo..."
BUILD_CMD="gcc $CFLAGS ${PROFILER_SRC[@]} -o ../../build/profiler_demo $LDFLAGS"
echo "$BUILD_CMD"

if $BUILD_CMD; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    echo "Run the demo:"
    echo "  ./build/profiler_demo"
    echo ""
    echo "Then open your browser to:"
    echo "  http://localhost:8080"
    echo ""
else
    echo ""
    echo "✗ Build failed"
    exit 1
fi
