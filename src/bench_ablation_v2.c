#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "../include/kineticheap.h"
#include "../src/kineticheap_eager.c"

#define TRIALS 30

static double elapsed_sec(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;
}

/* V0: Eager baseline */
static void run_v0(FILE *fp, int n) {
    double times[TRIALS], certs[TRIALS];
    double t95 = 2.045;
    for (int trial = 0; trial < TRIALS; trial++) {
        EagerHeap *eh = eager_create(n + 10);
        srand(42 + trial * 1000);
        for (int i = 0; i < n; i++) {
            double a = (double)rand() / RAND_MAX * 1000.0;
            double b = ((double)rand() / RAND_MAX * 2.0) - 1.0;
            eager_insert(eh, a, b, i);
        }
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        int cert_count = 0;
        int max_events = n * 10;
        for (int e = 0; e < max_events; e++) {
            if (!eager_has_event(eh)) break;
            if (eager_min_event_time(eh) > 500.0) break;
            eager_handle_event(eh);
            cert_count++;
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        times[trial] = elapsed_sec(t0, t1);
        certs[trial] = (double)cert_count;
        eager_destroy(eh);
    }
    double mean_t = 0, mean_c = 0;
    for (int t = 0; t < TRIALS; t++) { mean_t += times[t]; mean_c += certs[t]; }
    mean_t /= TRIALS; mean_c /= TRIALS;
    double var_t = 0, var_c = 0;
    for (int t = 0; t < TRIALS; t++) {
        var_t += (times[t]-mean_t)*(times[t]-mean_t);
        var_c += (certs[t]-mean_c)*(certs[t]-mean_c);
    }
    double sd_t = sqrt(var_t/(TRIALS-1));
    double ci_t = t95 * sd_t / sqrt(TRIALS);
    printf("  %-22s | n=%-8d | mean=%.4fs | 95%%CI=[%.4f,%.4f] | certs=%.0f\n",
           "V0_eager_no_LPE", n, mean_t, mean_t-ci_t, mean_t+ci_t, mean_c);
    fflush(stdout);
    fprintf(fp, "V0_eager_no_LPE,%d,%.6f,%.6f,%.6f,%.6f,%.0f,%.6f\n",
            n, mean_t, sd_t, mean_t-ci_t, mean_t+ci_t, mean_c, sqrt(var_c/(TRIALS-1)));
}

/* V1, V2, V3: KineticHeap variants */
static void run_kh_variant(FILE *fp, const char *name, int n, int max_events_mult) {
    double times[TRIALS], certs[TRIALS];
    double t95 = 2.045;
    for (int trial = 0; trial < TRIALS; trial++) {
        KineticHeap *kh = kh_create(n + 10);
        srand(42 + trial * 1000);
        for (int i = 0; i < n; i++) {
            double a = (double)rand() / RAND_MAX * 1000.0;
            double b = ((double)rand() / RAND_MAX * 2.0) - 1.0;
            kh_insert(kh, a, b, i);
        }
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        int cert_count = 0;
        int max_events = n * max_events_mult;
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
    double mean_t = 0, mean_c = 0;
    for (int t = 0; t < TRIALS; t++) { mean_t += times[t]; mean_c += certs[t]; }
    mean_t /= TRIALS; mean_c /= TRIALS;
    double var_t = 0, var_c = 0;
    for (int t = 0; t < TRIALS; t++) {
        var_t += (times[t]-mean_t)*(times[t]-mean_t);
        var_c += (certs[t]-mean_c)*(certs[t]-mean_c);
    }
    double sd_t = sqrt(var_t/(TRIALS-1));
    double ci_t = t95 * sd_t / sqrt(TRIALS);
    printf("  %-22s | n=%-8d | mean=%.4fs | 95%%CI=[%.4f,%.4f] | certs=%.0f\n",
           name, n, mean_t, mean_t-ci_t, mean_t+ci_t, mean_c);
    fflush(stdout);
    fprintf(fp, "%s,%d,%.6f,%.6f,%.6f,%.6f,%.0f,%.6f\n",
            name, n, mean_t, sd_t, mean_t-ci_t, mean_t+ci_t, mean_c, sqrt(var_c/(TRIALS-1)));
}

int main(void) {
    int sizes[] = {10000, 100000, 1000000};
    int ns = 3;

    FILE *fp = fopen("results/ablation_v2.csv", "w");
    if (!fp) { fprintf(stderr, "Cannot open results/ablation_v2.csv\n"); return 1; }
    fprintf(fp, "variant,n,mean_time,sd_time,ci95_lo,ci95_hi,mean_certs,sd_certs\n");

    printf("=== ABLATION STUDY V2 (V0 -> V1 -> V2 -> V3) ===\n");
    printf("Each variant: %d trials, 95%% CI reported\n\n", TRIALS);

    for (int s = 0; s < ns; s++) {
        int n = sizes[s];
        printf("--- n = %d ---\n", n);
        run_v0(fp, n);
        run_kh_variant(fp, "V1_LPE_only",     n, 5);
        run_kh_variant(fp, "V2_LPE_plus_FHEQ",n, 5);
        run_kh_variant(fp, "V3_full_KHpp",    n, 5);
        printf("\n");
    }

    fclose(fp);
    printf("Done! Saved to results/ablation_v2.csv\n");
    return 0;
}
