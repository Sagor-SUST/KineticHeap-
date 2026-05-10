# KineticHeap++ Reproducible Container
# Ubuntu 22.04 + GCC 12 + all benchmarks
# Usage:
#   docker build -t kineticheappp docker/
#   docker run --rm -v $(pwd)/results:/results kineticheappp

FROM ubuntu:22.04

LABEL maintainer="Md. Mahbobul Hasan Sagor <sagor@cse.sust.edu>"
LABEL description="Reproducible benchmark environment for KineticHeap++"
LABEL version="1.0"

# Install dependencies
RUN apt-get update && apt-get install -y \
    gcc \
    make \
    git \
    time \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /kineticheap

# Copy all source files
COPY . .

# Build all executables
RUN mkdir -p build results && \
    gcc -O3 -Wall -std=c11 -Iinclude \
        src/kineticheap.c src/main_bench.c \
        -o build/kineticheap -lm && \
    gcc -O3 -Wall -std=c11 -Iinclude \
        src/kineticheap.c src/bench_scale.c \
        -o build/bench_scale -lm && \
    gcc -O3 -Wall -std=c11 \
        src/baselines/naive_pq.c \
        -o build/baseline_naive -lm && \
    gcc -O3 -Wall -std=c11 \
        src/baselines/indexed_heap.c \
        -o build/baseline_indexed -lm && \
    gcc -O3 -Wall -std=c11 \
        src/baselines/stl_pq.c \
        -o build/baseline_stl -lm

# Default command: run full benchmark suite
CMD ["bash", "scripts/run_bench.sh"]