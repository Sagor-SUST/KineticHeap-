#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "kineticheap.h"

int main(void) {
    int sizes[] = {10000, 30000, 100000, 300000};
    int num_sizes = 4;
    FILE *fp = fopen("results/scaling.csv", "w");
    if (!fp) { fprintf(stderr, "Cannot open results/scaling.csv\n"); return 1; }

    fprintf(fp, "n,time_sec,cert_count\n");

    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        printf("Testing n=%d ...\n", n);
        fflush(stdout);

        KineticHeap *kh = kh_create(n + 10);
        srand(42);

        /* Insert n items */
        for (int i = 0; i < n; i++) {
            double a = (double)rand() / RAND_MAX * 1000.0;
            double b = ((double)rand() / RAND_MAX * 2.0) - 1.0;
            kh_insert(kh, a, b, i);
        }

        /* Time the event processing */
        clock_t start = clock();
        int cert_count = 0;
        while (kh->time < 50.0 && cert_count < n * 10) {
            kh_handle_event(kh);
            cert_count++;
        }
        clock_t end = clock();

        double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
        fprintf(fp, "%d,%.6f,%d\n", n, elapsed, cert_count);
        printf("  Done: %.4f sec, %d certificates\n", elapsed, cert_count);
        fflush(stdout);

        kh_destroy(kh);
    }

    fclose(fp);
    printf("\nResults saved to results/scaling.csv\n");
    return 0;
}