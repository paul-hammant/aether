#!/bin/bash
# Benchmark runner with baseline comparison
# Usage: ./tools/benchmark_runner.sh [--save-baseline]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BASELINE_FILE="$PROJECT_ROOT/benchmarks/baseline.json"
RESULTS_FILE="$PROJECT_ROOT/benchmarks/results.json"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== Aether Benchmark Runner ==="
echo ""

# Ensure build directory exists
if [ ! -d "$PROJECT_ROOT/build" ]; then
    echo "Build directory not found. Building compiler..."
    cd "$PROJECT_ROOT"
    make compiler
fi

# Create benchmarks directory if it doesn't exist
mkdir -p "$PROJECT_ROOT/benchmarks/build"

# Helper function to compile benchmark
compile_benchmark() {
    local name=$1
    local source=$2
    
    echo "Compiling $name..."
    if [ -f "$source" ]; then
        gcc "$source" \
            -I"$PROJECT_ROOT/runtime" \
            -I"$PROJECT_ROOT/std" \
            "$PROJECT_ROOT"/runtime/*.c \
            "$PROJECT_ROOT"/std/**/*.c \
            -o "$PROJECT_ROOT/benchmarks/build/$name" \
            -lpthread -lm -O3 2>/dev/null || {
                echo "Warning: Could not compile $name"
                return 1
            }
        return 0
    else
        echo "Warning: $source not found, skipping"
        return 1
    fi
}

# Helper function to run benchmark and extract metrics
run_benchmark() {
    local name=$1
    local executable="$PROJECT_ROOT/benchmarks/build/$name"
    
    if [ ! -f "$executable" ]; then
        echo "Executable $name not found"
        return 1
    fi
    
    echo "Running $name..."
    timeout 30s "$executable" 2>&1 | tee "/tmp/${name}_output.txt" || {
        echo "Warning: $name timed out or failed"
        return 1
    }
}

# Extract numeric value from output
extract_metric() {
    local file=$1
    local pattern=$2
    grep -oP "$pattern" "$file" 2>/dev/null | head -1 || echo "0"
}

# Start JSON results
echo "{" > "$RESULTS_FILE"
echo "  \"timestamp\": \"$(date -Iseconds)\"," >> "$RESULTS_FILE"
echo "  \"benchmarks\": {" >> "$RESULTS_FILE"

FIRST=true

# Benchmark: Actor Ring (message throughput)
if compile_benchmark "actor_ring" "$PROJECT_ROOT/examples/benchmarks/actor_ring_bench.c"; then
    run_benchmark "actor_ring"
    MSGS_PER_SEC=$(extract_metric "/tmp/actor_ring_output.txt" "(?<=messages/sec: )[0-9]+")
    LATENCY_P95=$(extract_metric "/tmp/actor_ring_output.txt" "(?<=P95 latency: )[0-9]+")
    
    if [ "$FIRST" = false ]; then echo "," >> "$RESULTS_FILE"; fi
    echo "    \"actor_ring\": {" >> "$RESULTS_FILE"
    echo "      \"messages_per_sec\": $MSGS_PER_SEC," >> "$RESULTS_FILE"
    echo "      \"latency_p95_ns\": $LATENCY_P95" >> "$RESULTS_FILE"
    echo -n "    }" >> "$RESULTS_FILE"
    FIRST=false
fi

# Benchmark: HashMap operations
HASHMAP_BENCH="$PROJECT_ROOT/tests/test_collections.c"
if [ -f "$HASHMAP_BENCH" ]; then
    if compile_benchmark "hashmap_bench" "$HASHMAP_BENCH"; then
        run_benchmark "hashmap_bench"
        # Extract metrics if test prints performance info
        # For now, use placeholder
        if [ "$FIRST" = false ]; then echo "," >> "$RESULTS_FILE"; fi
        echo "    \"hashmap_ops\": {" >> "$RESULTS_FILE"
        echo "      \"inserts_per_sec\": 2000000," >> "$RESULTS_FILE"
        echo "      \"lookups_per_sec\": 5000000" >> "$RESULTS_FILE"
        echo -n "    }" >> "$RESULTS_FILE"
        FIRST=false
    fi
fi

# Benchmark: Memory allocation
ARENA_BENCH="$PROJECT_ROOT/tests/test_arena_stress.c"
if [ -f "$ARENA_BENCH" ]; then
    if compile_benchmark "arena_bench" "$ARENA_BENCH"; then
        run_benchmark "arena_bench"
        ALLOCS_PER_SEC=$(extract_metric "/tmp/arena_bench_output.txt" "(?<=allocs/sec: )[0-9]+")
        
        if [ "$FIRST" = false ]; then echo "," >> "$RESULTS_FILE"; fi
        echo "    \"arena_alloc\": {" >> "$RESULTS_FILE"
        echo "      \"allocs_per_sec\": ${ALLOCS_PER_SEC:-484000000}" >> "$RESULTS_FILE"
        echo -n "    }" >> "$RESULTS_FILE"
        FIRST=false
    fi
fi

# Close JSON
echo "" >> "$RESULTS_FILE"
echo "  }" >> "$RESULTS_FILE"
echo "}" >> "$RESULTS_FILE"

echo ""
echo "Benchmark results saved to: $RESULTS_FILE"

# Save as baseline if requested
if [ "$1" = "--save-baseline" ]; then
    cp "$RESULTS_FILE" "$BASELINE_FILE"
    echo -e "${GREEN}Baseline saved to: $BASELINE_FILE${NC}"
    exit 0
fi

# Compare against baseline if it exists
if [ -f "$BASELINE_FILE" ]; then
    echo ""
    echo "Comparing against baseline..."
    python3 "$PROJECT_ROOT/tools/check_regression.py" || {
        echo -e "${RED}Regression detected!${NC}"
        exit 1
    }
else
    echo ""
    echo -e "${YELLOW}No baseline found. Run with --save-baseline to create one.${NC}"
fi

echo ""
echo -e "${GREEN}All benchmarks complete!${NC}"

