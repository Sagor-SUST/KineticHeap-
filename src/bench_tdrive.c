#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "../include/kineticheap.h"

#define TRIALS 30

static double elapsed_sec(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;
}

int main(void) {
    /* Load T-Drive data */
    FILE *fp = fopen("data/tdrive/tdrive_ab.txt", "r");
    if (!fp) { fprintf(stderr, "Cannot open tdrive_ab.txt\n"); return 1; }

    double *as = malloc(20000 * sizeof(double));
    double *bs = malloc(20000 * sizeof(double));
    int n = 0;
    while (fscanf(fp, "%lf %lf", &as[n], &bs[n]) == 2) n++;
    fclose(fp);
    printf("Loaded %d taxis from T-Drive\n", n);

    double t95 = 2.045;
    double kh_times[TRIALS], b2_times[TRIALS];
    double kh_certs[TRIALS], b2_certs[TRIALS];

    /* KH++ trials */
    printf("Running KH++ (%d trials)...\n", TRIALS);
    for (int trial = 0; trial < TRIALS; trial++) {
        KineticHeap *kh = kh_create(n + 10);
        for (int i = 0; i < n; i++) kh_insert(kh, as[i], bs[i], i);
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        int certs = 0;
        while (kh->fheap->min && kh->fheap->min->key < 1e9) {
            kh_handle_event(kh);
            certs++;
            if (certs > n * 20) break;
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        kh_times[trial] = elapsed_sec(t0, t1);
        kh_certs[trial] = certs;
        kh_destroy(kh);
        printf("."); fflush(stdout);
    }
    printf("\n");

    /* Compute KH++ stats */
    double kh_mean = 0, kh_cert_mean = 0;
    for (int i = 0; i < TRIALS; i++) { kh_mean += kh_times[i]; kh_cert_mean += kh_certs[i]; }
    kh_mean /= TRIALS; kh_cert_mean /= TRIALS;
    double kh_var = 0;
    for (int i = 0; i < TRIALS; i++) kh_var += (kh_times[i]-kh_mean)*(kh_times[i]-kh_mean);
    double kh_sd = sqrt(kh_var/(TRIALS-1));
    double kh_ci = t95 * kh_sd / sqrt(TRIALS);

    printf("KH++  mean=%.4fs SD=%.4f 95%%CI=[%.4f,%.4f] certs=%.0f\n",
           kh_mean, kh_sd, kh_mean-kh_ci, kh_mean+kh_ci, kh_cert_mean);

    /* Save results */
    FILE *out = fopen("results/tdrive_results.csv", "w");
    fprintf(out, "method,n,mean_time,sd_time,ci95_lo,ci95_hi,mean_certs\n");
    fprintf(out, "KH++,%d,%.6f,%.6f,%.6f,%.6f,%.0f\n",
            n, kh_mean, kh_sd, kh_mean-kh_ci, kh_mean+kh_ci, kh_cert_mean);
    fclose(out);

    printf("Saved to results/tdrive_results.csv\n");
    free(as); free(bs);
    return 0;
}
