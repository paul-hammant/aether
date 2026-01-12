#!/usr/bin/env bash
# Comprehensive Cross-Language Benchmark Suite
# Tests: Ping-Pong, Ring, Skynet patterns
# Languages: Aether, C++, Go, Rust, Pony, Scala

set -e

echo "================================================================================"
echo "  COMPREHENSIVE CROSS-LANGUAGE ACTOR BENCHMARK SUITE"
echo "================================================================================"

# Hardware info
if [[ "$OSTYPE" == "darwin"* ]]; then
    CPU=$(sysctl -n machdep.cpu.brand_string)
    CORES=$(sysctl -n hw.ncpu)
    OS="macOS"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    CPU=$(lscpu | grep "Model name" | cut -d: -f2 | xargs)
    CORES=$(nproc)
    OS="Linux"
else
    CPU="Unknown"
    CORES="Unknown"
    OS="$OSTYPE"
fi

TIMESTAMP=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

echo "CPU:   $CPU"
echo "Cores: $CORES"
echo "OS:    $OS"
echo "================================================================================"

# Test patterns
PATTERNS=("ping_pong" "ring" "skynet")

for PATTERN in "${PATTERNS[@]}"; do
    echo ""
    echo ""
    echo "### PATTERN: ${PATTERN^^} ###"
    echo ""

    # Initialize results JSON for this pattern
    cat > "visualize/results_${PATTERN}.json" <<EOF
{
  "pattern": "$PATTERN",
  "timestamp": "$TIMESTAMP",
  "hardware": {
    "cpu": "$CPU",
    "cores": $CORES,
    "os": "$OS"
  },
  "benchmarks": {}
}
EOF

    # Function to update pattern results
    update_pattern_results() {
        local pattern=$1
        local name=$2
        local msg_per_sec=$3
        local cycles_per_msg=$4
        local runtime=$5
        local notes=$6

        python3 -c "
import json
with open('visualize/results_${pattern}.json', 'r') as f:
    data = json.load(f)
data['benchmarks']['$name'] = {
    'runtime': '$runtime',
    'msg_per_sec': int($msg_per_sec),
    'cycles_per_msg': $cycles_per_msg,
    'notes': '$notes'
}
with open('visualize/results_${pattern}.json', 'w') as f:
    json.dump(data, f, indent=2)
"
    }

    # Aether
    if [ "$PATTERN" = "ping_pong" ]; then
        EXE="../../tests/runtime/bench_batched_atomic"
    else
        EXE="aether/${PATTERN}"
    fi

    if [ -f "$EXE" ]; then
        echo "=== Aether ($PATTERN) ==="
        OUTPUT=$($EXE 2>&1) || true
        echo "$OUTPUT"

        if [[ $OUTPUT =~ Throughput:[[:space:]]+([0-9]+)[[:space:]]*M[[:space:]]*msg/sec ]]; then
            MSG_PER_SEC=$(echo "${BASH_REMATCH[1]} * 1000000" | bc)
            CYCLES=0
            if [[ $OUTPUT =~ Cycles/msg:[[:space:]]+([0-9.]+) ]]; then
                CYCLES=${BASH_REMATCH[1]}
            fi
            update_pattern_results "$PATTERN" "Aether" "$MSG_PER_SEC" "$CYCLES" "Native C" "Batched atomic optimization"
        fi
    fi

    # C++
    CPP_EXE="cpp/${PATTERN}"
    if [ -f "$CPP_EXE" ]; then
        echo ""
        echo "=== C++ ($PATTERN) ==="
        OUTPUT=$($CPP_EXE 2>&1) || true
        echo "$OUTPUT"

        if [[ $OUTPUT =~ Throughput:[[:space:]]+([0-9]+)[[:space:]]*M[[:space:]]*msg/sec ]]; then
            MSG_PER_SEC=$(echo "${BASH_REMATCH[1]} * 1000000" | bc)
            CYCLES=0
            if [[ $OUTPUT =~ Cycles/msg:[[:space:]]+([0-9.]+) ]]; then
                CYCLES=${BASH_REMATCH[1]}
            fi
            update_pattern_results "$PATTERN" "C++" "$MSG_PER_SEC" "$CYCLES" "Native" "std::thread + std::atomic"
        fi
    fi

    # Go
    GO_EXE="go/${PATTERN}"
    if [ -f "$GO_EXE" ]; then
        echo ""
        echo "=== Go ($PATTERN) ==="
        OUTPUT=$($GO_EXE 2>&1) || true
        echo "$OUTPUT"

        if [[ $OUTPUT =~ Throughput:[[:space:]]+([0-9]+)[[:space:]]*M[[:space:]]*msg/sec ]]; then
            MSG_PER_SEC=$(echo "${BASH_REMATCH[1]} * 1000000" | bc)
            CYCLES=0
            if [[ $OUTPUT =~ Cycles/msg:[[:space:]]+([0-9.]+) ]]; then
                CYCLES=${BASH_REMATCH[1]}
            fi
            update_pattern_results "$PATTERN" "Go" "$MSG_PER_SEC" "$CYCLES" "Go runtime" "Goroutines + channels"
        fi
    fi

    # Rust
    if [ "$PATTERN" = "ping_pong" ]; then
        RUST_EXE="rust/target/release/ping_pong"
    else
        RUST_EXE="rust/target/release/${PATTERN}"
    fi

    if [ -f "$RUST_EXE" ]; then
        echo ""
        echo "=== Rust ($PATTERN) ==="
        OUTPUT=$($RUST_EXE 2>&1) || true
        echo "$OUTPUT"

        if [[ $OUTPUT =~ Throughput:[[:space:]]+([0-9]+)[[:space:]]*M[[:space:]]*msg/sec ]]; then
            MSG_PER_SEC=$(echo "${BASH_REMATCH[1]} * 1000000" | bc)
            CYCLES=0
            if [[ $OUTPUT =~ Cycles/msg:[[:space:]]+([0-9.]+) ]]; then
                CYCLES=${BASH_REMATCH[1]}
            fi
            update_pattern_results "$PATTERN" "Rust" "$MSG_PER_SEC" "$CYCLES" "Tokio async" "Async channels"
        fi
    fi
done

# Summary
echo ""
echo ""
echo "================================================================================"
echo "  SUMMARY - ALL PATTERNS"
echo "================================================================================"

for PATTERN in "${PATTERNS[@]}"; do
    if [ -f "visualize/results_${PATTERN}.json" ]; then
        echo ""
        echo "${PATTERN^^}:"
        python3 -c "
import json
with open('visualize/results_${PATTERN}.json', 'r') as f:
    data = json.load(f)
benchmarks = data.get('benchmarks', {})
sorted_benchmarks = sorted(benchmarks.items(), key=lambda x: x[1].get('msg_per_sec', 0), reverse=True)
for name, info in sorted_benchmarks:
    msg_per_sec = info.get('msg_per_sec', 0) // 1000000
    print(f'  {name:<15} {msg_per_sec:>8}M msg/sec')
"
    fi
done

# Use ping_pong as default for dashboard
if [ -f "visualize/results_ping_pong.json" ]; then
    cp "visualize/results_ping_pong.json" "visualize/results.json"
fi

echo ""
echo ""
echo "Results saved to visualize/results_*.json"
echo ""
echo "To view dashboard:"
echo "  cd visualize"
echo "  ./server"
echo "  Open http://localhost:8080"
