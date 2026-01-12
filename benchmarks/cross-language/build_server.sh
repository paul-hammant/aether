#!/usr/bin/env bash
# Build and run the Aether benchmark visualization server

echo "============================================================"
echo "  Building Aether Benchmark Visualization Server"
echo "============================================================"

set -e  # Exit on error
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/visualize"

# Build the server
echo ""
echo "Compiling server.c..."

# Collect all source files
NET_SOURCES="../../std/net/aether_http_server.c ../../std/net/aether_http.c ../../std/net/aether_net.c"
RUNTIME_SOURCES="../../runtime/scheduler/multicore_scheduler.c ../../runtime/scheduler/scheduler_optimizations.c"
RUNTIME_SOURCES="$RUNTIME_SOURCES ../../runtime/config/aether_optimization_config.c"
RUNTIME_SOURCES="$RUNTIME_SOURCES ../../runtime/memory/memory.c ../../runtime/memory/aether_arena.c"
RUNTIME_SOURCES="$RUNTIME_SOURCES ../../runtime/memory/aether_pool.c ../../runtime/memory/aether_memory_stats.c"
RUNTIME_SOURCES="$RUNTIME_SOURCES ../../runtime/memory/aether_arena_optimized.c ../../runtime/memory/aether_batch.c"
RUNTIME_SOURCES="$RUNTIME_SOURCES ../../runtime/utils/aether_tracing.c ../../runtime/utils/aether_bounds_check.c"
RUNTIME_SOURCES="$RUNTIME_SOURCES ../../runtime/utils/aether_test.c ../../runtime/utils/aether_cpu_detect.c"
RUNTIME_SOURCES="$RUNTIME_SOURCES ../../runtime/utils/aether_simd_vectorized.c"
RUNTIME_SOURCES="$RUNTIME_SOURCES ../../runtime/aether_runtime_types.c ../../runtime/aether_runtime.c"
RUNTIME_SOURCES="$RUNTIME_SOURCES ../../runtime/aether_numa.c ../../runtime/actors/aether_send_buffer.c"
STD_SOURCES="../../std/string/aether_string.c ../../std/fs/aether_fs.c ../../std/json/aether_json.c"

if gcc -O2 -o server server.c \
    -I../../std/net \
    -I../../std \
    -I../../runtime \
    -I../../runtime/config \
    -I../../runtime/scheduler \
    $NET_SOURCES $RUNTIME_SOURCES $STD_SOURCES \
    -lpthread 2>&1; then

    echo "✓ Server built successfully!"
    echo ""
    echo "Starting server on http://localhost:8080"
    echo "Press Ctrl+C to stop"
    echo ""

    # Run the server
    ./server
else
    echo "✗ Build failed!"
    exit 1
fi
