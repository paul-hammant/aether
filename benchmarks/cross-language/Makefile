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
	@echo ""
	@echo "=========================================="
	@echo "  Running Cross-Language Benchmarks"
	@echo "=========================================="
	@echo ""
	@./run_benchmarks.sh

# Build the visualization server
build-ui:
	@echo ""
	@echo "=========================================="
	@echo "  Building Visualization Server"
	@echo "=========================================="
	@echo ""
	@./build_server.sh 2>&1 | grep -v "Starting server" || true
	@pkill -f "visualize/server" 2>/dev/null || true
	@sleep 1

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
