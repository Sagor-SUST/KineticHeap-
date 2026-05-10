/* naive_pq.c
 * B1 — Naive Resorting Priority Queue
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

typedef struct {
    double intercept;
    double slope;
    int    id;
} Item;

typedef struct {
    Item  *items;
    int    size;
    int    capacity;
    double time;
} NaivePQ;

NaivePQ *npq_create(int capacity) {
    NaivePQ *pq  = (NaivePQ *)malloc(sizeof(NaivePQ));
    pq->items    = (Item *)malloc(capacity * sizeof(Item));
    pq->size     = 0;
    pq->capacity = capacity;
    pq->time     = 0.0;
    return pq;
}

void npq_insert(NaivePQ *pq, double intercept, double slope, int id) {
    if (pq->size >= pq->capacity) return;
    pq->items[pq->size].intercept = intercept;
    pq->items[pq->size].slope     = slope;
    pq->items[pq->size].id        = id;
    pq->size++;
}

int npq_find_min(NaivePQ *pq) {
    int    min_idx = 0;
    double min_key = pq->items[0].intercept
                   + pq->items[0].slope * pq->time;
    for (int i = 1; i < pq->size; i++) {
        double key = pq->items[i].intercept
                   + pq->items[i].slope * pq->time;
        if (key < min_key) { min_key = key; min_idx = i; }
    }
    return min_idx;
}

double npq_next_event(NaivePQ *pq) {
    double t_next = 1e18;
    for (int i = 0; i < pq->size - 1; i++) {
        for (int j = i + 1; j < pq->size; j++) {
            double db = pq->items[i].slope - pq->items[j].slope;
            if (fabs(db) < 1e-12) continue;
            double t = (pq->items[j].intercept
                      - pq->items[i].intercept) / db;
            if (t > pq->time + 1e-12 && t < t_next)
                t_next = t;
        }
    }
    return t_next;
}

void npq_destroy(NaivePQ *pq) {
    free(pq->items);
    free(pq);
}

int main(void) {
    int sizes[]   = {500, 1000, 2000, 3000};
    int num_sizes = 4;

    FILE *fp = fopen("results/baseline_naive.csv", "w");
    if (!fp) { fprintf(stderr, "Cannot open output file\n"); return 1; }
    fprintf(fp, "n,time_sec,cert_count\n");

    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        printf("Naive PQ: Testing n=%d ...\n", n); fflush(stdout);

        NaivePQ *pq = npq_create(n + 10);
        srand(42);

        for (int i = 0; i < n; i++) {
            double a = (double)rand() / RAND_MAX * 1000.0;
            double b = ((double)rand() / RAND_MAX * 2.0) - 1.0;
            npq_insert(pq, a, b, i);
        }

        clock_t start      = clock();
        int     cert_count = 0;

        for (int e = 0; e < 500; e++) {
            npq_find_min(pq);
            double t_next = npq_next_event(pq);
            if (t_next > 500.0 || t_next >= 1e17) break;
            pq->time = t_next;
            cert_count++;
        }

        clock_t end     = clock();
        double  elapsed = (double)(end - start) / CLOCKS_PER_SEC;

        fprintf(fp, "%d,%.6f,%d\n", n, elapsed, cert_count);
        printf("  Done: %.4f sec, %d events\n", elapsed, cert_count);
        fflush(stdout);

        npq_destroy(pq);
    }

    fclose(fp);
    printf("\nResults saved to results/baseline_naive.csv\n");
    return 0;
}