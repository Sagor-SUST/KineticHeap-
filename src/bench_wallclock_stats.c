#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "kineticheap.h"

#define TRIALS 30

static double rand_double(double lo, double hi) {
    return lo + (hi - lo) * ((double)rand() / RAND_MAX);
}

static double single_run(int n, unsigned int seed) {
    double *as = (double*)malloc(n * sizeof(double));
    double *bs = (double*)malloc(n * sizeof(double));
    srand(seed);
    for (int i = 0; i < n; i++) {
        as[i] = rand_double(0, 1000);
        bs[i] = rand_double(-1, 1);
    }
    KineticHeap *kh = kh_create(n + 10);
    for (int i = 0; i < n; i++) kh_insert(kh, as[i], bs[i], i);
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    int max_events = n * 5;
    for (int e = 0; e < max_events; e++) {
        if (!kh->fheap->min) break;
        if (kh->fheap->min->key > 500.0) break;
        kh_handle_event(kh);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) * 1e-9;
    kh_destroy(kh);
    free(as); free(bs);
    return elapsed;
}

static void run_stats(int n, FILE *fp) {
    double times[TRIALS];
    printf("  KH++ n=%-8d trials: ", n); fflush(stdout);
    for (int t = 0; t < TRIALS; t++) {
        times[t] = single_run(n, 42 + t * 1000);
        printf("."); fflush(stdout);
    }
    printf("\n");
    double sum = 0.0;
    for (int t = 0; t < TRIALS; t++) sum += times[t];
    double mean = sum / TRIALS;
    double var = 0.0;
    for (int t = 0; t < TRIALS; t++) { double d = times[t] - mean; var += d*d; }
    double sd = sqrt(var / (TRIALS - 1));
    double ci = 1.96 * sd / sqrt((double)TRIALS);
    printf("    mean=%.4fs  SD=%.4f  95%%CI=[%.4f,%.4f]\n",
           mean, sd, mean-ci, mean+ci);
    fprintf(fp, "KH++,%d,%.6f,%.6f,%.6f,%.6f\n",
            n, mean, sd, mean-ci, mean+ci);
    fflush(fp);
}

int main(void) {
    int sizes[] = {10000, 30000, 100000, 300000, 1000000, 3000000, 10000000};
    int ns = 7;
    FILE *fp = fopen("results/wallclock_stats.csv", "w");
    if (!fp) { fprintf(stderr, "Cannot open results/wallclock_stats.csv\n"); return 1; }
    fprintf(fp, "method,n,mean_time,sd_time,ci_lo,ci_hi\n");
    printf("=== Wall-clock 30-trial Stats (KH++) ===\n\n");
    for (int s = 0; s < ns; s++) run_stats(sizes[s], fp);
    fclose(fp);
    printf("\nDone! Saved to results/wallclock_stats.csv\n");
    return 0;
}
