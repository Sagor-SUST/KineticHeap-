CC      = gcc
CFLAGS  = -O3 -march=native -Wall -Wextra -std=c11 -Iinclude
LDFLAGS = -lm

SRC     = src/kineticheap.c src/main_bench.c
OBJ     = build/kineticheap.o build/main_bench.o
TARGET  = build/kineticheap

.PHONY: all clean debug bench

all: build $(TARGET)

build:
	mkdir -p build

build/kineticheap.o: src/kineticheap.c include/kineticheap.h
	$(CC) $(CFLAGS) -c src/kineticheap.c -o build/kineticheap.o

build/main_bench.o: src/main_bench.c include/kineticheap.h
	$(CC) $(CFLAGS) -c src/main_bench.c -o build/main_bench.o

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LDFLAGS)

debug: CFLAGS += -DKH_DEBUG -g -O0
debug: all

bench: all
	@echo "Running benchmark..."
	./build/kineticheap

clean:
	rm -rf build