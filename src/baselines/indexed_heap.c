/* indexed_heap.c
 * B4 — Indexed Binary Heap (cache-friendly, no Fibonacci heap)
 * Standard binary min-heap with position tracking.
 * No lazy evaluation, no certificate queue.
 * Re-evaluates ALL keys at every event — O(n log n) per step.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

typedef struct {
    double intercept;
    double slope;
    int    id;
} IHNode;

typedef struct {
    IHNode *nodes;
    int    *pos;
    int     size;
    int     capacity;
    double  time;
} IndexedHeap;

IndexedHeap *ih_create(int capacity) {
    IndexedHeap *h = (IndexedHeap *)malloc(sizeof(IndexedHeap));
    h->nodes    = (IHNode *)calloc(capacity + 1, sizeof(IHNode));
    h->pos      = (int *)calloc(capacity + 1, sizeof(int));
    h->size     = 0;
    h->capacity = capacity;
    h->time     = 0.0;
    return h;
}

static double ih_key(IndexedHeap *h, int idx) {
    return h->nodes[idx].intercept + h->nodes[idx].slope * h->time;
}

static void ih_swap(IndexedHeap *h, int i, int j) {
    IHNode tmp   = h->nodes[i];
    h->nodes[i]  = h->nodes[j];
    h->nodes[j]  = tmp;
    h->pos[h->nodes[i].id] = i;
    h->pos[h->nodes[j].id] = j;
}

static void ih_sift_up(IndexedHeap *h, int idx) {
    while (idx > 1 && ih_key(h, idx) < ih_key(h, idx / 2)) {
        ih_swap(h, idx, idx / 2);
        idx /= 2;
    }
}

static void ih_sift_down(IndexedHeap *h, int idx) {
    int n = h->size;
    while (1) {
        int smallest = idx;
        int l = 2 * idx, r = 2 * idx + 1;
        if (l <= n && ih_key(h, l) < ih_key(h, smallest)) smallest = l;
        if (r <= n && ih_key(h, r) < ih_key(h, smallest)) smallest = r;
        if (smallest == idx) break;
        ih_swap(h, idx, smallest);
        idx = smallest;
    }
}

void ih_insert(IndexedHeap *h, double intercept, double slope, int id) {
    if (h->size >= h->capacity) return;
    h->size++;
    h->nodes[h->size].intercept = intercept;
    h->nodes[h->size].slope     = slope;
    h->nodes[h->size].id        = id;
    h->pos[id] = h->size;
    ih_sift_up(h, h->size);
}

/* Rebuild entire heap after time advances — O(n) */
void ih_rebuild(IndexedHeap *h) {
    for (int i = h->size / 2; i >= 1; i--)
        ih_sift_down(h, i);
}

/* Find next crossing time — O(n^2) but only adjacent pairs */
double ih_next_event(IndexedHeap *h) {
    double t_next = 1e18;
    for (int i = 1; i <= h->size; i++) {
        int l = 2 * i, r = 2 * i + 1;
        for (int j = l; j <= r && j <= h->size; j++) {
            double db = h->nodes[i].slope - h->nodes[j].slope;
            if (fabs(db) < 1e-12) continue;
            double t = (h->nodes[j].intercept
                    - h->nodes[i].intercept) / db;
            if (t > h->time + 1e-12 && t < t_next)
                t_next = t;
        }
    }
    return t_next;
}

void ih_destroy(IndexedHeap *h) {
    free(h->nodes);
    free(h->pos);
    free(h);
}

int main(void) {
int sizes[]   = {10000, 30000, 100000};
int num_sizes = 3;

    FILE *fp = fopen("results/baseline_indexed.csv", "w");
    if (!fp) { fprintf(stderr, "Cannot open output file\n"); return 1; }
    fprintf(fp, "n,time_sec,cert_count\n");

    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        printf("Indexed Heap: Testing n=%d ...\n", n); fflush(stdout);

        IndexedHeap *h = ih_create(n + 10);
        srand(42);

        for (int i = 0; i < n; i++) {
            double a = (double)rand() / RAND_MAX * 1000.0;
            double b = ((double)rand() / RAND_MAX * 2.0) - 1.0;
            ih_insert(h, a, b, i);
        }

        clock_t start      = clock();
        int     cert_count = 0;
        int     max_events = n * 5;

        for (int e = 0; e < max_events; e++) {
            double t_next = ih_next_event(h);
            if (t_next > 500.0 || t_next >= 1e17) break;
            h->time = t_next;
            ih_rebuild(h);
            cert_count++;
        }

        clock_t end     = clock();
        double  elapsed = (double)(end - start) / CLOCKS_PER_SEC;

        fprintf(fp, "%d,%.6f,%d\n", n, elapsed, cert_count);
        printf("  Done: %.4f sec, %d events\n", elapsed, cert_count);
        fflush(stdout);

        ih_destroy(h);
    }

    fclose(fp);
    printf("\nResults saved to results/baseline_indexed.csv\n");
    return 0;
}