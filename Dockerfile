# Aether Programming Language - Docker Image
# Usage:
#   docker build -t aether .
#   docker run -it aether
#   docker run -v $(pwd)/myproject:/work aether aetherc /work/main.ae /work/output.c

FROM gcc:12-bullseye

LABEL maintainer="Aether Development Team"
LABEL description="Aether Programming Language Compiler and Runtime"
LABEL version="0.4.0"

# Install dependencies
RUN apt-get update && apt-get install -y \
    make \
    git \
    python3 \
    python3-pip \
    valgrind \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /aether

# Copy source code
COPY . .

# Build compiler and tools
RUN make -j$(nproc) compiler && \
    make lsp && \
    make profiler && \
    make stdlib

# Add compiler to PATH
ENV PATH="/aether/build:${PATH}"

# Create workspace directory
WORKDIR /work

# Default command: show help
CMD ["aetherc", "--help"]

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD aetherc --version || exit 1

# Metadata
EXPOSE 8080
VOLUME ["/work"]

# Example usage:
# docker run -it aether bash
# docker run -v $(pwd):/work aether aetherc /work/program.ae /work/output.c
# docker run -p 8080:8080 aether /aether/build/profiler_demo

