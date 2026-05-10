#!/bin/bash
# run_bench.sh — End-to-end benchmark runner for KineticHeap++
# Usage: bash scripts/run_bench.sh

set -e

echo "========================================"
echo "  KineticHeap++ Benchmark Suite"
echo "========================================"
echo ""

# Create directories
mkdir -p build results

# Compile all
echo "[1/5] Compiling KineticHeap++..."
gcc -O3 -Wall -std=c11 -Iinclude \
    src/kineticheap.c src/main_bench.c \
    -o build/kineticheap -lm

echo "[2/5] Compiling scaling benchmark..."
gcc -O3 -Wall -std=c11 -Iinclude \
    src/kineticheap.c src/bench_scale.c \
    -o build/bench_scale -lm

echo "[3/5] Compiling baselines..."
gcc -O3 -Wall -std=c11 src/baselines/naive_pq.c    -o build/baseline_naive   -lm
gcc -O3 -Wall -std=c11 src/baselines/indexed_heap.c -o build/baseline_indexed -lm
gcc -O3 -Wall -std=c11 src/baselines/stl_pq.c       -o build/baseline_stl     -lm

echo ""
echo "========================================"
echo "  Running Correctness Test"
echo "========================================"
./build/kineticheap

echo ""
echo "========================================"
echo "  Running KineticHeap++ Scaling Benchmark"
echo "========================================"
./build/bench_scale

echo ""
echo "========================================"
echo "  Running Baseline: Naive PQ (B1)"
echo "========================================"
./build/baseline_naive

echo ""
echo "========================================"
echo "  Running Baseline: Indexed Heap (B4)"
echo "========================================"
./build/baseline_indexed

echo ""
echo "========================================"
echo "  Running Baseline: STL Heap (B8)"
echo "========================================"
./build/baseline_stl

echo ""
echo "========================================"
echo "  All benchmarks complete!"
echo "  Results saved in results/"
echo "========================================"
ls -lh results/