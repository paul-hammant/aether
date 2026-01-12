#!/usr/bin/env bash
# Cross-Language Benchmark Runner for Unix/Linux/macOS
# Compiles and runs benchmarks, generates results.json

set -e

echo "============================================================"
echo "  Cross-Language Actor Benchmark Suite"
echo "============================================================"

# Get hardware info
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
echo "============================================================"

# Initialize results JSON
cat > visualize/results.json <<EOF
{
  "timestamp": "$TIMESTAMP",
  "hardware": {
    "cpu": "$CPU",
    "cores": $CORES,
    "os": "$OS"
  },
  "benchmarks": {
  }
}
EOF

# Function to update results JSON
update_results() {
    local name=$1
    local msg_per_sec=$2
    local cycles_per_msg=$3
    local runtime=$4
    local notes=$5

    # Use Python or jq to update JSON (Python is more portable)
    python3 -c "
import json
with open('visualize/results.json', 'r') as f:
    data = json.load(f)
data['benchmarks']['$name'] = {
    'runtime': '$runtime',
    'msg_per_sec': int($msg_per_sec),
    'cycles_per_msg': $cycles_per_msg,
    'notes': '$notes'
}
with open('visualize/results.json', 'w') as f:
    json.dump(data, f, indent=2)
"
}

run_benchmark() {
    local name=$1
    local command=$2
    local build_command=$3

    echo ""
    echo "=== Benchmarking $name ==="

    if [ -n "$build_command" ]; then
        echo "Building..."
        if eval "$build_command" > /dev/null 2>&1; then
            :
        else
            echo "✗ Build failed"
            return 1
        fi
    fi

    echo "Running..."
    if output=$(eval "$command" 2>&1); then
        echo "$output"

        # Parse output
        msg_per_sec=0
        cycles_per_msg=0

        if [[ $output =~ Throughput:[[:space:]]+([0-9.]+)[[:space:]]*M[[:space:]]*msg/sec ]]; then
            msg_per_sec=$(echo "${BASH_REMATCH[1]} * 1000000" | bc)
        fi

        if [[ $output =~ Cycles/msg:[[:space:]]+([0-9.]+) ]]; then
            cycles_per_msg=${BASH_REMATCH[1]}
        fi

        if (( $(echo "$msg_per_sec > 0" | bc -l) )); then
            echo "✓ $name : $(echo "scale=0; $msg_per_sec/1000000" | bc)M msg/sec"
            return 0
        fi
    else
        echo "✗ Failed to run $name"
        return 1
    fi
}

# Benchmark Aether
AETHER_EXE="../../tests/runtime/bench_batched_atomic"
if [ -f "$AETHER_EXE" ]; then
    if run_benchmark "Aether" "$AETHER_EXE"; then
        update_results "Aether" "$msg_per_sec" "$cycles_per_msg" "Native C" "Batched atomic updates"
    fi
else
    echo ""
    echo "⚠ Aether benchmark not found at $AETHER_EXE"
fi

# Benchmark C++
CPP_SRC="cpp/ping_pong.cpp"
CPP_EXE="cpp/ping_pong"
if [ -f "$CPP_SRC" ]; then
    if run_benchmark "C++" "$CPP_EXE" "g++ -O3 -std=c++17 -march=native -o $CPP_EXE $CPP_SRC -lpthread"; then
        update_results "C++" "$msg_per_sec" "$cycles_per_msg" "Native" "std::thread + std::atomic"
    fi
fi

# Benchmark Go
GO_SRC="go/ping_pong.go"
if [ -f "$GO_SRC" ]; then
    if command -v go &> /dev/null; then
        if run_benchmark "Go" "go/ping_pong" "cd go && go build -o ping_pong ping_pong.go && cd .."; then
            update_results "Go" "$msg_per_sec" "$cycles_per_msg" "Go runtime" "Goroutines + channels"
        fi
    else
        echo ""
        echo "⚠ Go not installed, skipping Go benchmark"
    fi
fi

# Benchmark Rust
RUST_SRC="rust/Cargo.toml"
if [ -f "$RUST_SRC" ]; then
    if command -v cargo &> /dev/null; then
        if run_benchmark "Rust" "rust/target/release/ping_pong" "cd rust && cargo build --release > /dev/null 2>&1 && cd .."; then
            update_results "Rust" "$msg_per_sec" "$cycles_per_msg" "Tokio async" "Async channels"
        fi
    else
        echo ""
        echo "⚠ Rust not installed, skipping Rust benchmark"
    fi
fi

# Display summary
echo ""
echo "============================================================"
echo "  Summary"
echo "============================================================"

# Sort and display results using Python
python3 -c "
import json
with open('visualize/results.json', 'r') as f:
    data = json.load(f)
benchmarks = data.get('benchmarks', {})
sorted_benchmarks = sorted(benchmarks.items(), key=lambda x: x[1].get('msg_per_sec', 0), reverse=True)
for name, info in sorted_benchmarks:
    msg_per_sec = info.get('msg_per_sec', 0) // 1000000
    print(f'{name:<15} {msg_per_sec:>8}M msg/sec')
"

echo ""
echo "✓ Results saved to visualize/results.json"
echo ""
echo "To view results:"
echo "  cd visualize"
echo "  ./server"
echo "  Open http://localhost:8080"
