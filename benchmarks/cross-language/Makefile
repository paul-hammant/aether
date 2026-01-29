# Aether Cross-Language Benchmark Suite

.PHONY: all benchmark-ui run-benchmarks build-ui clean help

# Default target
all: benchmark-ui

# Main target: Run benchmarks and launch UI
benchmark-ui: run-benchmarks build-ui
	@echo ""
	@echo "=========================================="
	@echo "  Launching Benchmark Visualization UI"
	@echo "=========================================="
	@echo ""
	@echo "Open your browser at http://localhost:8080"
	@echo "Press Ctrl+C to stop the server"
	@echo ""
	@cd visualize && ./server

# Run all benchmarks
run-benchmarks:
	@./run_benchmarks.sh

# Build the visualization server
build-ui:
	@lsof -ti :8080 | xargs kill -9 2>/dev/null || true
	@pkill -9 -f "visualize/server" 2>/dev/null || true
	@sleep 1
	@cd visualize && gcc -O2 -o server server.c \
		-I../../../std/net -I../../../std -I../../../std/io -I../../../std/fs \
		-I../../../std/json -I../../../std/string -I../../../std/collections \
		-I../../../runtime -I../../../runtime/config -I../../../runtime/scheduler \
		-I../../../runtime/actors -I../../../runtime/memory -I../../../runtime/utils \
		../../../std/net/aether_http_server.c ../../../std/net/aether_http.c ../../../std/net/aether_net.c \
		../../../runtime/scheduler/multicore_scheduler.c ../../../runtime/scheduler/scheduler_optimizations.c \
		../../../runtime/config/aether_optimization_config.c \
		../../../runtime/memory/memory.c ../../../runtime/memory/aether_arena.c \
		../../../runtime/memory/aether_pool.c ../../../runtime/memory/aether_memory_stats.c \
		../../../runtime/memory/aether_arena_optimized.c ../../../runtime/memory/aether_batch.c \
		../../../runtime/utils/aether_tracing.c ../../../runtime/utils/aether_bounds_check.c \
		../../../runtime/utils/aether_test.c ../../../runtime/utils/aether_cpu_detect.c \
		../../../runtime/utils/aether_simd_vectorized.c \
		../../../runtime/aether_runtime_types.c ../../../runtime/aether_runtime.c \
		../../../runtime/aether_numa.c ../../../runtime/actors/aether_send_buffer.c \
		../../../runtime/actors/aether_actor.c \
		../../../runtime/actors/aether_actor_thread.c ../../../runtime/actors/aether_message_registry.c \
		../../../std/string/aether_string.c ../../../std/json/aether_json.c \
		../../../std/io/aether_io.c ../../../std/collections/aether_collections.c \
		../../../std/collections/aether_vector.c ../../../std/collections/aether_hashmap.c \
		-lpthread 2>&1 | grep -v "warning" || true

# Clean all build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -f visualize/server
	@echo "Done!"

# Help target
help:
	@echo "Aether Cross-Language Benchmark Suite"
	@echo ""
	@echo "Targets:"
	@echo "  make benchmark-ui   - Run all benchmarks and launch visualization (default)"
	@echo "  make run-benchmarks - Only run benchmarks, don't launch UI"
	@echo "  make build-ui       - Only build the visualization server"
	@echo "  make clean          - Clean all build artifacts"
	@echo "  make help           - Show this help message"
