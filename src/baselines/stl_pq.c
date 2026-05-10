/* stl_pq.c
 * B8 — Standard Binary Heap (C equivalent of std::priority_queue)
 * No position tracking, no lazy evaluation, no certificate queue.
 * Full rebuild at every event.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

typedef struct {
    double intercept;
    double slope;
    int    id;
} Node;

typedef struct {
    Node  *data;
    int    size;
    int    capacity;
    double time;
} BinaryHeap;

BinaryHeap *bh_create(int capacity) {
    BinaryHeap *h = (BinaryHeap *)malloc(sizeof(BinaryHeap));
    h->data     = (Node *)calloc(capacity + 1, sizeof(Node));
    h->size     = 0;
    h->capacity = capacity;
    h->time     = 0.0;
    return h;
}

static double bh_key(BinaryHeap *h, int i) {
    return h->data[i].intercept + h->data[i].slope * h->time;
}

static void bh_swap(BinaryHeap *h, int i, int j) {
    Node tmp  = h->data[i];
    h->data[i] = h->data[j];
    h->data[j] = tmp;
}

static void bh_sift_up(BinaryHeap *h, int i) {
    while (i > 1 && bh_key(h, i) < bh_key(h, i / 2)) {
        bh_swap(h, i, i / 2);
        i /= 2;
    }
}

static void bh_sift_down(BinaryHeap *h, int i) {
    int n = h->size;
    while (1) {
        int s = i, l = 2*i, r = 2*i+1;
        if (l <= n && bh_key(h, l) < bh_key(h, s)) s = l;
        if (r <= n && bh_key(h, r) < bh_key(h, s)) s = r;
        if (s == i) break;
        bh_swap(h, i, s);
        i = s;
    }
}

void bh_insert(BinaryHeap *h, double intercept, double slope, int id) {
    if (h->size >= h->capacity) return;
    h->size++;
    h->data[h->size].intercept = intercept;
    h->data[h->size].slope     = slope;
    h->data[h->size].id        = id;
    bh_sift_up(h, h->size);
}

/* Rebuild after time advances — O(n) */
void bh_rebuild(BinaryHeap *h) {
    for (int i = h->size / 2; i >= 1; i--)
        bh_sift_down(h, i);
}

/* Next event: check adjacent pairs only — O(n) */
double bh_next_event(BinaryHeap *h) {
    double t_next = 1e18;
    for (int i = 1; i <= h->size; i++) {
        int l = 2*i, r = 2*i+1;
        for (int j = l; j <= r && j <= h->size; j++) {
            double db = h->data[i].slope - h->data[j].slope;
            if (fabs(db) < 1e-12) continue;
            double t = (h->data[j].intercept
                      - h->data[i].intercept) / db;
            if (t > h->time + 1e-12 && t < t_next)
                t_next = t;
        }
    }
    return t_next;
}

void bh_destroy(BinaryHeap *h) {
    free(h->data);
    free(h);
}

int main(void) {
    int sizes[]   = {10000, 30000, 100000};
    int num_sizes = 3;

    FILE *fp = fopen("results/baseline_stl.csv", "w");
    if (!fp) { fprintf(stderr, "Cannot open output file\n"); return 1; }
    fprintf(fp, "n,time_sec,cert_count\n");

    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        printf("STL Heap: Testing n=%d ...\n", n); fflush(stdout);

        BinaryHeap *h = bh_create(n + 10);
        srand(42);

        for (int i = 0; i < n; i++) {
            double a = (double)rand() / RAND_MAX * 1000.0;
            double b = ((double)rand() / RAND_MAX * 2.0) - 1.0;
            bh_insert(h, a, b, i);
        }

        clock_t start      = clock();
        int     cert_count = 0;
        int     max_events = n * 5;

        for (int e = 0; e < max_events; e++) {
            double t_next = bh_next_event(h);
            if (t_next > 500.0 || t_next >= 1e17) break;
            h->time = t_next;
            bh_rebuild(h);
            cert_count++;
        }

        clock_t end     = clock();
        double  elapsed = (double)(end - start) / CLOCKS_PER_SEC;

        fprintf(fp, "%d,%.6f,%d\n", n, elapsed, cert_count);
        printf("  Done: %.4f sec, %d events\n", elapsed, cert_count);
        fflush(stdout);

        bh_destroy(h);
    }

    fclose(fp);
    printf("\nResults saved to results/baseline_stl.csv\n");
    return 0;
}