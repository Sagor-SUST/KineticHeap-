# KineticHeap++

> **A Cache-Efficient Kinetic Priority Queue for Dynamic Linear Trajectories**

[![Build Status](https://github.com/Sagor-SUST/KineticHeap-/actions/workflows/ci.yml/badge.svg)](https://github.com/Sagor-SUST/KineticHeap-/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows%20%7C%20macOS-blue)]()
[![Language](https://img.shields.io/badge/language-C11-orange)]()

---

## Overview

**KineticHeap++** is a practical, high-performance kinetic priority queue for *n* dynamic
priorities evolving as linear functions of time **kᵢ(t) = aᵢ + bᵢt**.

This repository contains the full C11 implementation accompanying the paper:

> Md. Mahbobul Hasan Sagor, Mominul Islam, Nasimul Bari Nafim, and Tawseef Zaman Turjo,
> **"KINETICHEAP++: Engineering a Cache-Efficient Kinetic Priority Queue for Dynamic Linear Trajectories,"**
> *IEEE Transactions on Computers*, 2026 (under review).

### Key Design Features

| Component | Description |
|---|---|
| **Lazy Priority Evaluation (LPE)** | Defers key updates until needed — reduces evaluations from O(n) to O(1) per event |
| **Fibonacci-Heap Event Queue (FHEQ)** | Amortised O(1) INSERT and DECREASE-KEY for certificate management |
| **Interleaved Array Layout** | Cache-friendly struct-of-arrays — parent and children share the same 64-byte cache line |

---

## Repository Structure

```
KineticHeap/
├── include/
│   └── kineticheap.h       # Public API and data structures
├── src/
│   ├── kineticheap.c       # Core implementation
│   ├── main_bench.c        # Small correctness test (n=10)
│   ├── bench_scale.c       # Scaling benchmark (n=10K to 1M)
│   └── baselines/          # Baseline implementations (coming soon)
├── results/
│   └── scaling.csv         # Raw benchmark output
├── datasets/               # Synthetic and real-world datasets
├── scripts/
│   └── run_bench.sh        # End-to-end benchmark runner
├── docker/
│   └── Dockerfile          # Reproducible container
├── Makefile
├── LICENSE
└── README.md
```

---

## Build Instructions

### Requirements

- GCC 9+ (tested with GCC 12.2.0)
- GNU Make
- `-lm` (math library)

### Linux / macOS

```bash
git clone https://github.com/Sagor-SUST/KineticHeap-.git
cd KineticHeap-
make
```

### Windows (GCC via MinGW / VS Code)

```powershell
gcc -O3 -Wall -std=c11 -Iinclude src/kineticheap.c src/main_bench.c -o build/kineticheap.exe -lm
./build/kineticheap.exe
```

---

## Quick Start

### Correctness Test (n = 10)

```powershell
gcc -O3 -Wall -std=c11 -Iinclude src/kineticheap.c src/main_bench.c -o build/kineticheap.exe -lm
./build/kineticheap.exe
```

Expected output:
```
=== KINETICHEAP++ Test ===
Insert id=0  a=0.534  b=-0.976
...
Heap size: 10
Min key at t=0: 0.5341
Next event at t=4.071034  (i=5, j=10)
Processing one event...
Done! Time is now: 4.071034
Min key now: -3.4376
All OK!
```

### Scaling Benchmark (n = 10K to 300K)

```powershell
gcc -O3 -Wall -std=c11 -Iinclude src/kineticheap.c src/bench_scale.c -o build/bench_scale.exe -lm
./build/bench_scale.exe
```

Results are saved to `results/scaling.csv`.

---

## API Reference

```c
/* Create a kinetic heap with given capacity */
KineticHeap *kh_create(int capacity);

/* Insert item with linear trajectory ki(t) = intercept + slope * t */
void kh_insert(KineticHeap *kh, double intercept, double slope, int id);

/* Process the next certificate event (advances global time) */
void kh_handle_event(KineticHeap *kh);

/* Return the current minimum key value */
double kh_min_key(KineticHeap *kh);

/* Remove and return the minimum element */
HeapNode kh_delete_min(KineticHeap *kh);

/* Free all memory */
void kh_destroy(KineticHeap *kh);
```

---

## Benchmark Results

> Results collected on: Windows 10, GCC 6.3.0, -O3, Intel Core i5, 8 GB RAM.
> 
> *(Full results on Linux server will be added upon paper acceptance.)*

| n | Time (sec) | Certificate Count |
|---|---|---|
| 10,000 | TBD | TBD |
| 30,000 | TBD | TBD |
| 100,000 | TBD | TBD |
| 300,000 | TBD | TBD |

*Table will be updated with real benchmark output.*

---

## Reproducibility

All results in the paper are reproducible using fixed random seeds (`srand(42)`).

```powershell
# Run full benchmark suite
./build/bench_scale.exe

# Output saved to:
results/scaling.csv
```

---

## Authors

| Name | Affiliation |
|---|---|
| Md. Mahbobul Hasan Sagor | Dept. of CSE, Shahjalal University of Science & Technology (SUST), Sylhet, Bangladesh |
| Mominul Islam | Dept. of CSE, SUST |
| Nasimul Bari Nafim | Dept. of CSE, SUST |
| Tawseef Zaman Turjo | Dept. of CSE, SUST |

---

## Citation

If you use this code in your research, please cite:

```bibtex
@article{sagor2026kineticheap,
  title     = {{KINETICHEAP}++: Engineering a Cache-Efficient Kinetic Priority Queue
               for Dynamic Linear Trajectories},
  author    = {Sagor, Md. Mahbobul Hasan and Islam, Mominul and
               Nafim, Nasimul Bari and Turjo, Tawseef Zaman},
  journal   = {IEEE Transactions on Computers},
  year      = {2026},
  note      = {Under review}
}
```

---

## License

This project is licensed under the MIT License — see [LICENSE](LICENSE) for details.
