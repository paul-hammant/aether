#!/usr/bin/env bash
# Build Stress Test

echo "Building Profiler Stress Test"

if ! command -v gcc &> /dev/null; then
    echo "ERROR: gcc not found"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

mkdir -p ../../build

SOURCES=(
    "profiler_server.c"
    "stress_test.c"
    "../../runtime/memory/aether_arena.c"
    "../../runtime/memory/aether_pool.c"
    "../../runtime/memory/aether_memory_stats.c"
    "../../runtime/utils/aether_tracing.c"
)

CFLAGS="-O2 -I../../runtime -I../../runtime/memory -I../../runtime/utils -I../../std -Wall -Wextra -Wno-unused-parameter -DAETHER_PROFILING"
LDFLAGS="-pthread"

echo "Compiling..."
BUILD_CMD="gcc $CFLAGS ${SOURCES[@]} -o ../../build/stress_test $LDFLAGS"

if $BUILD_CMD; then
    echo ""
    echo "Build successful!"
    echo ""
    echo "To run:"
    echo "  ./build/stress_test"
    echo ""
else
    echo "Build failed"
    exit 1
fi
