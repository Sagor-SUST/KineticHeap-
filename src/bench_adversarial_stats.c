#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "kineticheap.h"

#define TRIALS 30

static double rand_double(double lo, double hi) {
    return lo + (hi - lo) * ((double)rand() / RAND_MAX);
}
static double rand_gauss(double mu, double sigma) {
    double u1 = ((double)rand() + 1) / ((double)RAND_MAX + 1);
    double u2 = ((double)rand() + 1) / ((double)RAND_MAX + 1);
    return mu + sigma * sqrt(-2.0 * log(u1)) * cos(2.0 * 3.14159265358979 * u2);
}
static void single_run(int n, double *as, double *bs,
                       double *out_time, long *out_certs) {
    KineticHeap *kh = kh_create(n + 10);
    for (int i = 0; i < n; i++) kh_insert(kh, as[i], bs[i], i);
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    long certs = 0;
    int max_events = n * 5;
    for (int e = 0; e < max_events; e++) {
        if (!kh->fheap->min) break;
        if (kh->fheap->min->key > 500.0) break;
        kh_handle_event(kh);
        certs++;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    *out_time = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) * 1e-9;
    *out_certs = certs;
    kh_destroy(kh);
}
static void run_stats(const char *name, int n, unsigned int base_seed,
                      void (*fill)(int, double*, double*, unsigned int),
                      FILE *fp) {
    double times[TRIALS];
    long certs_arr[TRIALS];
    double *as = (double*)malloc(n * sizeof(double));
    double *bs = (double*)malloc(n * sizeof(double));
    printf("  %-12s n=%-8d trials: ", name, n); fflush(stdout);
    for (int t = 0; t < TRIALS; t++) {
        fill(n, as, bs, base_seed + t * 1000);
        single_run(n, as, bs, &times[t], &certs_arr[t]);
        printf("."); fflush(stdout);
    }
    printf("\n");
    double sum_t = 0.0; long sum_c = 0;
    for (int t = 0; t < TRIALS; t++) { sum_t += times[t]; sum_c += certs_arr[t]; }
    double mean_t = sum_t / TRIALS;
    double mean_c = (double)sum_c / TRIALS;
    double var_t = 0.0;
    for (int t = 0; t < TRIALS; t++) { double d = times[t] - mean_t; var_t += d*d; }
    double sd_t = sqrt(var_t / (TRIALS - 1));
    double ci_t = 1.96 * sd_t / sqrt((double)TRIALS);
    printf("    mean=%.4fs  SD=%.4f  95%%CI=[%.4f,%.4f]  certs=%.0f\n",
           mean_t, sd_t, mean_t - ci_t, mean_t + ci_t, mean_c);
    fprintf(fp, "%s,%d,%.6f,%.6f,%.6f,%.6f,%.0f\n",
            name, n, mean_t, sd_t, mean_t - ci_t, mean_t + ci_t, mean_c);
    fflush(fp);
    free(as); free(bs);
}
static void fill_random(int n, double *as, double *bs, unsigned int seed) {
    srand(seed);
    for (int i = 0; i < n; i++) { as[i]=rand_double(0,1000); bs[i]=rand_double(-1,1); }
}
static void fill_clustered(int n, double *as, double *bs, unsigned int seed) {
    srand(seed);
    for (int i = 0; i < n; i++) {
        as[i]=rand_double(0,1000);
        double mu=(i%2==0)?0.5:-0.5;
        bs[i]=rand_gauss(mu,0.05);
    }
}
static void fill_bursty(int n, double *as, double *bs, unsigned int seed) {
    srand(seed);
    for (int i = 0; i < n; i++) {
        as[i]=rand_double(0,1000);
        bs[i]=(rand()%10==0)?rand_double(-10,10):rand_double(-0.01,0.01);
    }
}
static void fill_degenerate(int n, double *as, double *bs, unsigned int seed) {
    srand(seed);
    for (int i = 0; i < n; i++) { as[i]=rand_double(0,1000); bs[i]=rand_double(-1,1)*1e-3; }
}
int main(void) {
    int sizes[] = {10000, 100000, 1000000};
    int ns = 3;
    FILE *fp = fopen("results/adversarial_stats.csv", "w");
    if (!fp) { fprintf(stderr, "Cannot open results/adversarial_stats.csv\n"); return 1; }
    fprintf(fp, "workload,n,mean_time,sd_time,ci_lo,ci_hi,mean_certs\n");
    printf("=== Adversarial Workload - %d Trials, 95%% CI ===\n\n", TRIALS);
    for (int s = 0; s < ns; s++) {
        int n = sizes[s];
        printf("--- n = %d ---\n", n);
        run_stats("random",     n, 42,   fill_random,     fp);
        run_stats("clustered",  n, 1042, fill_clustered,  fp);
        run_stats("bursty",     n, 2042, fill_bursty,     fp);
        run_stats("degenerate", n, 3042, fill_degenerate, fp);
        printf("\n");
    }
    fclose(fp);
    printf("Done! Results saved to results/adversarial_stats.csv\n");
    return 0;
}
