#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "kineticheap.h"

#define TRIALS 30

static double elapsed_sec(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;
}

int main(void) {
    int sizes[]   = {10000, 30000, 100000, 300000, 1000000, 3000000, 10000000};
    int num_sizes = 7;

    FILE *fp = fopen("results/scaling_stats.csv", "w");
    if (!fp) { fprintf(stderr, "Cannot open results/scaling_stats.csv\n"); return 1; }
    fprintf(fp, "n,mean_time,sd_time,ci95_lo,ci95_hi,mean_certs,sd_certs\n");

    /* t-value for 95% CI, df=29 */
    double t95 = 2.045;

    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        printf("Testing n=%d  (%d trials)...\n", n, TRIALS); fflush(stdout);

        double times[TRIALS];
        double certs[TRIALS];

        for (int trial = 0; trial < TRIALS; trial++) {
            KineticHeap *kh = kh_create(n + 10);
            /* different seed each trial but reproducible */
            srand(42 + trial * 1000);

            for (int i = 0; i < n; i++) {
                double a = (double)rand() / RAND_MAX * 1000.0;
                double b = ((double)rand() / RAND_MAX * 2.0) - 1.0;
                kh_insert(kh, a, b, i);
            }

            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);

            int cert_count = 0;
            int max_events = n * 5;
            for (int e = 0; e < max_events; e++) {
                if (!kh->fheap->min) break;
                if (kh->fheap->min->key > 500.0) break;
                kh_handle_event(kh);
                cert_count++;
            }

            clock_gettime(CLOCK_MONOTONIC, &t1);
            times[trial] = elapsed_sec(t0, t1);
            certs[trial] = (double)cert_count;

            kh_destroy(kh);
        }

        /* --- compute mean --- */
        double mean_t = 0, mean_c = 0;
        for (int t = 0; t < TRIALS; t++) { mean_t += times[t]; mean_c += certs[t]; }
        mean_t /= TRIALS;
        mean_c /= TRIALS;

        /* --- compute SD --- */
        double var_t = 0, var_c = 0;
        for (int t = 0; t < TRIALS; t++) {
            var_t += (times[t] - mean_t) * (times[t] - mean_t);
            var_c += (certs[t] - mean_c) * (certs[t] - mean_c);
        }
        double sd_t = sqrt(var_t / (TRIALS - 1));
        double sd_c = sqrt(var_c / (TRIALS - 1));

        /* --- 95% CI --- */
        double ci_t  = t95 * sd_t / sqrt(TRIALS);
        double lo    = mean_t - ci_t;
        double hi    = mean_t + ci_t;

        printf("  n=%-9d | mean=%.4fs | SD=%.4f | 95%%CI=[%.4f, %.4f] | certs=%.0f\n",
               n, mean_t, sd_t, lo, hi, mean_c);
        fflush(stdout);

        fprintf(fp, "%d,%.6f,%.6f,%.6f,%.6f,%.0f,%.0f\n",
                n, mean_t, sd_t, lo, hi, mean_c, sd_c);
    }

    fclose(fp);
    printf("\nDone! Results saved to results/scaling_stats.csv\n");
    return 0;
}
