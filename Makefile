CC      = gcc
CFLAGS  = -O3 -march=native -Wall -Wextra -std=c11 -Iinclude
LDFLAGS = -lm

.PHONY: all clean debug bench scale

# ─── Build directory ───────────────────────────────────────
all: build build/kineticheap build/bench_scale

build:
	mkdir -p build

# ─── Object files ──────────────────────────────────────────
build/kineticheap.o: src/kineticheap.c include/kineticheap.h
	$(CC) $(CFLAGS) -c src/kineticheap.c -o build/kineticheap.o

build/main_bench.o: src/main_bench.c include/kineticheap.h
	$(CC) $(CFLAGS) -c src/main_bench.c -o build/main_bench.o

build/bench_scale.o: src/bench_scale.c include/kineticheap.h
	$(CC) $(CFLAGS) -c src/bench_scale.c -o build/bench_scale.o

# ─── Executables ───────────────────────────────────────────
build/kineticheap: build/kineticheap.o build/main_bench.o
	$(CC) $(CFLAGS) build/kineticheap.o build/main_bench.o -o build/kineticheap $(LDFLAGS)

build/bench_scale: build/kineticheap.o build/bench_scale.o
	$(CC) $(CFLAGS) build/kineticheap.o build/bench_scale.o -o build/bench_scale $(LDFLAGS)

# ─── Shortcuts ─────────────────────────────────────────────
debug: CFLAGS += -DKH_DEBUG -g -O0
debug: all

bench: build/kineticheap
	@echo "Running correctness test..."
	./build/kineticheap

scale: build/bench_scale
	@echo "Running scaling benchmark..."
	./build/bench_scale

# ─── Clean ─────────────────────────────────────────────────
clean:
	rm -rf build