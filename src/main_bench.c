#include "../include/kineticheap.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double rand_double(double lo, double hi) {
    return lo + (hi - lo) * ((double)rand() / (double)RAND_MAX);
}

int main(void) {
    printf("=== KINETICHEAP++ Test ===\n\n");
    fflush(stdout);

    srand(42);
    int n = 10;
    KineticHeap *kh = kh_create(n + 10);

    /* insert 10 elements */
    for (int i = 0; i < n; i++) {
        double intercept = rand_double(0.0, 100.0);
        double slope     = rand_double(-1.0, 1.0);
        printf("Insert id=%d  a=%.3f  b=%.3f\n", i, intercept, slope);
        kh_insert(kh, intercept, slope, i);
    }

    printf("\nHeap size: %d\n", kh_size(kh));
    printf("Min key at t=0: %.4f\n\n", kh_min_key(kh));
    fflush(stdout);

    /* check event queue */
    if (kh->fheap->min == NULL) {
        printf("Event queue is EMPTY — no certificates!\n");
    } else {
        printf("Next event at t=%.6f  (i=%d, j=%d)\n",
               kh->fheap->min->key,
               kh->fheap->min->heap_i,
               kh->fheap->min->heap_j);
    }
    fflush(stdout);

    /* process just ONE event */
    printf("\nProcessing one event...\n");
    fflush(stdout);
    kh_handle_event(kh);
    printf("Done! Time is now: %.6f\n", kh->time);
    printf("Min key now: %.4f\n", kh_min_key(kh));
    fflush(stdout);

    kh_destroy(kh);
    printf("\nAll OK!\n");
    return 0;
}