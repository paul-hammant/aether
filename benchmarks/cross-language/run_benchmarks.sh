#!/usr/bin/env bash
# Main benchmark runner - ONLY REAL RESULTS, NO SYNTHETIC DATA
# If a benchmark can't run, it's simply skipped

set -e
cd "$(dirname "$0")"

# Source cargo environment if it exists
[ -f "$HOME/.cargo/env" ] && source "$HOME/.cargo/env"

# Add common tool paths
export PATH="$HOME/.cargo/bin:$PATH"

echo "============================================"
echo "  Running Cross-Language Benchmarks"
echo "============================================"
echo ""

RESULTS_FILE="visualize/results_ping_pong.json"
FIRST_RESULT=true

# Create results file header
cat > "$RESULTS_FILE" <<EOF
{
  "timestamp": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
  "pattern": "ping_pong",
  "hardware": {
    "cpu": "$(sysctl -n machdep.cpu.brand_string 2>/dev/null || lscpu | grep 'Model name' | cut -d: -f2 | xargs || echo 'Unknown')",
    "cores": $(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo '8'),
    "os": "$(uname -s)"
  },
  "benchmarks": {
EOF

# Helper to add comma before result (except first one)
add_result() {
    if [ "$FIRST_RESULT" = false ]; then
        echo "," >> "$RESULTS_FILE"
    fi
    FIRST_RESULT=false
}

# 0. Aether
echo "Building Aether ping-pong..."
if [ -f "aether/ping_pong.ae" ]; then
    if (cd aether && make clean &>/dev/null && make ping_pong &>/dev/null); then
        AETHER_OUTPUT=$(cd aether && /usr/bin/time -l ./ping_pong 2>&1 || time -v ./ping_pong 2>&1)
        AETHER_CYCLES=$(echo "$AETHER_OUTPUT" | grep "Cycles/msg" | awk '{print $2}')
        AETHER_THROUGHPUT=$(echo "$AETHER_OUTPUT" | grep "Throughput" | awk '{print $2}')
        AETHER_MSG_PER_SEC=$(echo "$AETHER_THROUGHPUT * 1000000" | bc 2>/dev/null | awk '{printf "%.0f", $0}')

        AETHER_MEMORY=$(echo "$AETHER_OUTPUT" | grep -E "maximum resident set size|Maximum resident" | awk '{print $1}' | head -1)
        AETHER_MEMORY_MB=$(echo "scale=2; $AETHER_MEMORY / 1024 / 1024" | bc 2>/dev/null | awk '{printf "%.2f", ($0 == "" ? 0 : $0)}')

        add_result
        cat >> "$RESULTS_FILE" <<EOF
    "Aether": {
      "runtime": "Native (Aether runtime)",
      "msg_per_sec": $AETHER_MSG_PER_SEC,
      "cycles_per_msg": $AETHER_CYCLES,
      "memory_mb": $AETHER_MEMORY_MB,
      "notes": "Lock-free SPSC queues, batched sends"
    }
EOF
        echo "✓ Aether: ${AETHER_THROUGHPUT}M msg/sec (${AETHER_MEMORY_MB}MB)"
    else
        echo "✗ Aether: Build failed (skipped)"
    fi
else
    echo "✗ Aether: Not found (skipped)"
fi

# 1. C (pthread) - Baseline
echo "Building C ping-pong..."
if (cd c && make clean &>/dev/null && make ping_pong &>/dev/null); then
    C_OUTPUT=$(cd c && /usr/bin/time -l ./ping_pong 2>&1 || time -v ./ping_pong 2>&1)
    C_CYCLES=$(echo "$C_OUTPUT" | grep "Cycles/msg" | awk '{print $2}')
    C_THROUGHPUT=$(echo "$C_OUTPUT" | grep "Throughput" | awk '{print $2}')
    C_MSG_PER_SEC=$(echo "$C_THROUGHPUT * 1000000" | bc 2>/dev/null | awk '{printf "%.0f", $0}')

    # Memory (macOS vs Linux)
    C_MEMORY=$(echo "$C_OUTPUT" | grep -E "maximum resident set size|Maximum resident" | awk '{print $1}' | head -1)
    C_MEMORY_MB=$(echo "scale=2; $C_MEMORY / 1024 / 1024" | bc 2>/dev/null | awk '{printf "%.2f", ($0 == "" ? 0 : $0)}')

    add_result
    cat >> "$RESULTS_FILE" <<EOF
    "C (pthread)": {
      "runtime": "Native (pthread)",
      "msg_per_sec": $C_MSG_PER_SEC,
      "cycles_per_msg": $C_CYCLES,
      "memory_mb": $C_MEMORY_MB,
      "notes": "pthread mutex + condvar (baseline)"
    }
EOF
    echo "✓ C (pthread): ${C_THROUGHPUT}M msg/sec (${C_MEMORY_MB}MB)"
else
    echo "✗ C (pthread): Build failed (skipped)"
fi

# 2. C++ (std::mutex + condition_variable)
echo "Building C++ ping-pong..."
if (cd cpp && g++ -O3 -std=c++17 -march=native ping_pong.cpp -o ping_pong -pthread &>/dev/null); then
    CPP_OUTPUT=$(cd cpp && /usr/bin/time -l ./ping_pong 2>&1 || time -v ./ping_pong 2>&1)
    CPP_CYCLES=$(echo "$CPP_OUTPUT" | grep "Cycles/msg" | awk '{print $2}')
    CPP_THROUGHPUT=$(echo "$CPP_OUTPUT" | grep "Throughput" | awk '{print $2}')
    CPP_MSG_PER_SEC=$(echo "$CPP_THROUGHPUT * 1000000" | bc 2>/dev/null | awk '{printf "%.0f", $0}')

    CPP_MEMORY=$(echo "$CPP_OUTPUT" | grep -E "maximum resident set size|Maximum resident" | awk '{print $1}' | head -1)
    CPP_MEMORY_MB=$(echo "scale=2; $CPP_MEMORY / 1024 / 1024" | bc 2>/dev/null | awk '{printf "%.2f", ($0 == "" ? 0 : $0)}')

    add_result
    cat >> "$RESULTS_FILE" <<EOF
    "C++": {
      "runtime": "Native (std::thread)",
      "msg_per_sec": $CPP_MSG_PER_SEC,
      "cycles_per_msg": $CPP_CYCLES,
      "memory_mb": $CPP_MEMORY_MB,
      "notes": "std::mutex + std::condition_variable (fair)"
    }
EOF
    echo "✓ C++: ${CPP_THROUGHPUT}M msg/sec (${CPP_MEMORY_MB}MB)"
else
    echo "✗ C++: Build failed (skipped)"
fi

# 3. Go
echo "Running Go ping-pong..."
if command -v go &> /dev/null; then
    GO_OUTPUT=$(cd go && /usr/bin/time -l go run ping_pong.go 2>&1 || (go run ping_pong.go 2>&1))
    GO_CYCLES=$(echo "$GO_OUTPUT" | grep "Cycles/msg" | awk '{print $2}')
    GO_THROUGHPUT=$(echo "$GO_OUTPUT" | grep "Throughput" | awk '{print $2}')
    GO_MSG_PER_SEC=$(echo "$GO_THROUGHPUT * 1000000" | bc 2>/dev/null | awk '{printf "%.0f", $0}')

    GO_MEMORY=$(echo "$GO_OUTPUT" | grep -E "maximum resident set size|Maximum resident" | awk '{print $1}' | head -1)
    GO_MEMORY_MB=$(echo "scale=2; $GO_MEMORY / 1024 / 1024" | bc 2>/dev/null | awk '{printf "%.2f", ($0 == "" ? 0 : $0)}')

    add_result
    cat >> "$RESULTS_FILE" <<EOF
    "Go": {
      "runtime": "Go runtime",
      "msg_per_sec": $GO_MSG_PER_SEC,
      "cycles_per_msg": $GO_CYCLES,
      "memory_mb": $GO_MEMORY_MB,
      "notes": "Goroutines with channels"
    }
EOF
    echo "✓ Go: ${GO_THROUGHPUT}M msg/sec (${GO_MEMORY_MB}MB)"
else
    echo "✗ Go: Not installed (skipped)"
fi

# 4. Rust
if command -v cargo &> /dev/null; then
    echo "Building Rust ping-pong..."
    if (cd rust && cargo build --release --bin ping_pong &>/dev/null); then
        RUST_OUTPUT=$(cd rust && /usr/bin/time -l ./target/release/ping_pong 2>&1 || time -v ./target/release/ping_pong 2>&1)
        RUST_CYCLES=$(echo "$RUST_OUTPUT" | grep "Cycles/msg" | awk '{print $2}')
        RUST_THROUGHPUT=$(echo "$RUST_OUTPUT" | grep "Throughput" | awk '{print $2}')
        RUST_MSG_PER_SEC=$(echo "$RUST_THROUGHPUT * 1000000" | bc 2>/dev/null | awk '{printf "%.0f", $0}')

        RUST_MEMORY=$(echo "$RUST_OUTPUT" | grep -E "maximum resident set size|Maximum resident" | awk '{print $1}' | head -1)
        RUST_MEMORY_MB=$(echo "scale=2; $RUST_MEMORY / 1024 / 1024" | bc 2>/dev/null | awk '{printf "%.2f", ($0 == "" ? 0 : $0)}')

        add_result
        cat >> "$RESULTS_FILE" <<EOF
    "Rust": {
      "runtime": "Native (Tokio async)",
      "msg_per_sec": $RUST_MSG_PER_SEC,
      "cycles_per_msg": $RUST_CYCLES,
      "memory_mb": $RUST_MEMORY_MB,
      "notes": "Tokio mpsc channels"
    }
EOF
        echo "✓ Rust: ${RUST_THROUGHPUT}M msg/sec (${RUST_MEMORY_MB}MB)"
    else
        echo "✗ Rust: Build failed (skipped)"
    fi
else
    echo "✗ Rust: Cargo not installed (skipped)"
fi

# 5. Java
if command -v javac &> /dev/null && command -v java &> /dev/null; then
    echo "Building Java ping-pong..."
    if (cd java && javac PingPong.java &>/dev/null); then
        JAVA_OUTPUT=$(cd java && /usr/bin/time -l java PingPong 2>&1 || time -v java PingPong 2>&1)
        JAVA_CYCLES=$(echo "$JAVA_OUTPUT" | grep "Cycles/msg" | awk '{print $2}')
        JAVA_THROUGHPUT=$(echo "$JAVA_OUTPUT" | grep "Throughput" | awk '{print $2}')
        JAVA_MSG_PER_SEC=$(echo "$JAVA_THROUGHPUT * 1000000" | bc 2>/dev/null | awk '{printf "%.0f", $0}')

        JAVA_MEMORY=$(echo "$JAVA_OUTPUT" | grep -E "maximum resident set size|Maximum resident" | awk '{print $1}' | head -1)
        JAVA_MEMORY_MB=$(echo "scale=2; $JAVA_MEMORY / 1024 / 1024" | bc 2>/dev/null | awk '{printf "%.2f", ($0 == "" ? 0 : $0)}')

        add_result
        cat >> "$RESULTS_FILE" <<EOF
    "Java": {
      "runtime": "JVM",
      "msg_per_sec": $JAVA_MSG_PER_SEC,
      "cycles_per_msg": $JAVA_CYCLES,
      "memory_mb": $JAVA_MEMORY_MB,
      "notes": "ArrayBlockingQueue"
    }
EOF
        echo "✓ Java: ${JAVA_THROUGHPUT}M msg/sec (${JAVA_MEMORY_MB}MB)"
    else
        echo "✗ Java: Build failed (skipped)"
    fi
else
    echo "✗ Java: Not installed (skipped)"
fi

# 6. Zig (if available)
if command -v zig &> /dev/null; then
    echo "Building Zig ping-pong..."
    if [ -f "zig/build.zig" ] && (cd zig && zig build -Doptimize=ReleaseFast &>/dev/null); then
        ZIG_OUTPUT=$(cd zig && /usr/bin/time -l ./zig-out/bin/ping_pong 2>&1 || time -v ./zig-out/bin/ping_pong 2>&1)
        ZIG_CYCLES=$(echo "$ZIG_OUTPUT" | grep "Cycles/msg" | awk '{print $2}')
        ZIG_THROUGHPUT=$(echo "$ZIG_OUTPUT" | grep "Throughput" | awk '{print $2}')
        ZIG_MSG_PER_SEC=$(echo "$ZIG_THROUGHPUT * 1000000" | bc 2>/dev/null | awk '{printf "%.0f", $0}')

        add_result
        cat >> "$RESULTS_FILE" <<EOF
    "Zig": {
      "runtime": "Native",
      "msg_per_sec": $ZIG_MSG_PER_SEC,
      "cycles_per_msg": $ZIG_CYCLES,
      "notes": "std::Thread with Mutex"
    }
EOF
        echo "✓ Zig: ${ZIG_THROUGHPUT}M msg/sec"
    else
        echo "✗ Zig: Build failed (skipped)"
    fi
else
    echo "✗ Zig: Not installed (skipped)"
fi

# 7. Elixir (if available)
if command -v elixir &> /dev/null; then
    echo "Running Elixir ping-pong..."
    if [ -f "elixir/ping_pong.exs" ]; then
        ELIXIR_OUTPUT=$(cd elixir && elixir ping_pong.exs 2>&1)
        ELIXIR_CYCLES=$(echo "$ELIXIR_OUTPUT" | grep "Cycles/msg" | awk '{print $2}')
        ELIXIR_THROUGHPUT=$(echo "$ELIXIR_OUTPUT" | grep "Throughput" | awk '{print $2}')
        ELIXIR_MSG_PER_SEC=$(echo "$ELIXIR_THROUGHPUT * 1000000" | bc 2>/dev/null | awk '{printf "%.0f", $0}')

        add_result
        cat >> "$RESULTS_FILE" <<EOF
    "Elixir": {
      "runtime": "BEAM VM",
      "msg_per_sec": $ELIXIR_MSG_PER_SEC,
      "cycles_per_msg": $ELIXIR_CYCLES,
      "notes": "Native process messaging"
    }
EOF
        echo "✓ Elixir: ${ELIXIR_THROUGHPUT}M msg/sec"
    else
        echo "✗ Elixir: Benchmark not found (skipped)"
    fi
else
    echo "✗ Elixir: Not installed (skipped)"
fi

# 8. Erlang (if available)
if command -v erlc &> /dev/null; then
    echo "Building Erlang ping-pong..."
    if [ -f "erlang/ping_pong.erl" ] && (cd erlang && erlc ping_pong.erl &>/dev/null); then
        ERLANG_OUTPUT=$(cd erlang && erl -noshell -s ping_pong main -s init stop 2>&1)
        ERLANG_CYCLES=$(echo "$ERLANG_OUTPUT" | grep "Cycles/msg" | awk '{print $2}')
        ERLANG_THROUGHPUT=$(echo "$ERLANG_OUTPUT" | grep "Throughput" | awk '{print $2}')
        ERLANG_MSG_PER_SEC=$(echo "$ERLANG_THROUGHPUT * 1000000" | bc 2>/dev/null | awk '{printf "%.0f", $0}')

        add_result
        cat >> "$RESULTS_FILE" <<EOF
    "Erlang": {
      "runtime": "BEAM VM",
      "msg_per_sec": $ERLANG_MSG_PER_SEC,
      "cycles_per_msg": $ERLANG_CYCLES,
      "notes": "Native process messaging"
    }
EOF
        echo "✓ Erlang: ${ERLANG_THROUGHPUT}M msg/sec"
    else
        echo "✗ Erlang: Build failed (skipped)"
    fi
else
    echo "✗ Erlang: Not installed (skipped)"
fi

# 9. Pony (if available)
if command -v ponyc &> /dev/null; then
    echo "Building Pony ping-pong..."
    if [ -f "pony/ping_pong.pony" ] && (cd pony && ponyc ping_pong.pony &>/dev/null); then
        PONY_OUTPUT=$(cd pony && ./ping_pong 2>&1)
        PONY_CYCLES=$(echo "$PONY_OUTPUT" | grep "Cycles/msg" | awk '{print $2}')
        PONY_THROUGHPUT=$(echo "$PONY_OUTPUT" | grep "Throughput" | awk '{print $2}')
        PONY_MSG_PER_SEC=$(echo "$PONY_THROUGHPUT * 1000000" | bc 2>/dev/null | awk '{printf "%.0f", $0}')

        add_result
        cat >> "$RESULTS_FILE" <<EOF
    "Pony": {
      "runtime": "Native (Pony runtime)",
      "msg_per_sec": $PONY_MSG_PER_SEC,
      "cycles_per_msg": $PONY_CYCLES,
      "notes": "Actor model language"
    }
EOF
        echo "✓ Pony: ${PONY_THROUGHPUT}M msg/sec"
    else
        echo "✗ Pony: Build failed (skipped)"
    fi
else
    echo "✗ Pony: Not installed (skipped)"
fi

# 10. Scala (if available)
if command -v scala &> /dev/null && command -v scalac &> /dev/null; then
    echo "Building Scala ping-pong..."
    if [ -f "scala/ping_pong.scala" ] && (cd scala && scalac ping_pong.scala &>/dev/null); then
        SCALA_OUTPUT=$(cd scala && scala PingPong 2>&1)
        SCALA_CYCLES=$(echo "$SCALA_OUTPUT" | grep "Cycles/msg" | awk '{print $2}')
        SCALA_THROUGHPUT=$(echo "$SCALA_OUTPUT" | grep "Throughput" | awk '{print $2}')
        SCALA_MSG_PER_SEC=$(echo "$SCALA_THROUGHPUT * 1000000" | bc 2>/dev/null | awk '{printf "%.0f", $0}')

        add_result
        cat >> "$RESULTS_FILE" <<EOF
    "Scala": {
      "runtime": "JVM (Scala)",
      "msg_per_sec": $SCALA_MSG_PER_SEC,
      "cycles_per_msg": $SCALA_CYCLES,
      "notes": "Scala futures"
    }
EOF
        echo "✓ Scala: ${SCALA_THROUGHPUT}M msg/sec"
    else
        echo "✗ Scala: Build failed (skipped)"
    fi
else
    echo "✗ Scala: Not installed (skipped)"
fi

# Close JSON
cat >> "$RESULTS_FILE" <<EOF

  }
}
EOF

echo ""
echo "============================================"
echo "Results saved to: $RESULTS_FILE"
echo "============================================"
echo ""
echo "Note: Only real benchmark results included."
echo "Languages that couldn't run were skipped."
