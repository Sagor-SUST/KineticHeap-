#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAX_N 11000000
#define TRIALS 30

static double heap[MAX_N];
static int heap_size;

static inline void heap_push(double val) {
    int i = ++heap_size;
    heap[i] = val;
    while (i > 1 && heap[i] < heap[i/2]) {
        double tmp = heap[i]; heap[i] = heap[i/2]; heap[i/2] = tmp;
        i /= 2;
    }
}

static inline double heap_pop(void) {
    double ret = heap[1];
    heap[1] = heap[heap_size--];
    int i = 1;
    while (1) {
        int smallest = i, l = 2*i, r = 2*i+1;
        if (l <= heap_size && heap[l] < heap[smallest]) smallest = l;
        if (r <= heap_size && heap[r] < heap[smallest]) smallest = r;
        if (smallest == i) break;
        double tmp = heap[i]; heap[i] = heap[smallest]; heap[smallest] = tmp;
        i = smallest;
    }
    return ret;
}

static double run_once(int n, unsigned int *seed) {
    heap_size = 0;
    for (int i = 0; i < n; i++) {
        *seed = *seed * 1664525u + 1013904223u;
        heap_push((double)(*seed) / 4294967296.0);
    }
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < n; i++) heap_pop();
    clock_gettime(CLOCK_MONOTONIC, &t1);
    return (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) * 1e-9;
}

int main(void) {
    int sizes[] = {10000, 30000, 100000, 300000, 1000000, 3000000, 10000000};
    int ns = 7;
    FILE *fp = fopen("results/binaryheap_stats.csv", "w");
    fprintf(fp, "n,mean,sd,ci_lo,ci_hi\n");
    printf("=== BINARY HEAP BASELINE (30 trials, 95%% CI) ===\n");
    for (int s = 0; s < ns; s++) {
        int n = sizes[s];
        double times[TRIALS];
        unsigned int seed = 42;
        printf("Testing n=%d (%d trials)...\n", n, TRIALS);
        for (int t = 0; t < TRIALS; t++) times[t] = run_once(n, &seed);
        double mean = 0;
        for (int t = 0; t < TRIALS; t++) mean += times[t];
        mean /= TRIALS;
        double sd = 0;
        for (int t = 0; t < TRIALS; t++) sd += (times[t]-mean)*(times[t]-mean);
        sd = sqrt(sd / (TRIALS-1));
        double ci = 1.96 * sd / sqrt(TRIALS);
        printf("  n=%-10d | mean=%.4fs | SD=%.4f | 95%%CI=[%.4f, %.4f]\n",
               n, mean, sd, mean-ci, mean+ci);
        fprintf(fp, "%d,%.6f,%.6f,%.6f,%.6f\n", n, mean, sd, mean-ci, mean+ci);
    }
    fclose(fp);
    printf("Done! Saved to results/binaryheap_stats.csv\n");
    return 0;
}
