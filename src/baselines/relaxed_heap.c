/*
 * B5 Baseline: Relaxed Heap Priority Queue
 * KineticHeap++ paper — baseline comparison
 * Windows + Linux compatible timing
 *
 * Relaxed heap: binary heap with lazy deletion.
 * Matches Fibonacci-heap amortised bounds with simpler implementation.
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
static double get_time(void) {
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (double)cnt.QuadPart / (double)freq.QuadPart;
}
#else
#include <time.h>
static double get_time(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}
#endif

/* ── Node ─────────────────────────────────────────────────────────────────── */
typedef struct {
    int    id;
    double key;
    int    deleted;   /* lazy deletion flag */
} RHNode;

/* ── Heap ─────────────────────────────────────────────────────────────────── */
typedef struct {
    RHNode *data;
    int     size;
    int     capacity;
} RelaxedHeap;

static RelaxedHeap *rh_create(int capacity) {
    RelaxedHeap *h = (RelaxedHeap *)malloc(sizeof(RelaxedHeap));
    h->data        = (RHNode *)malloc(capacity * sizeof(RHNode));
    h->size        = 0;
    h->capacity    = capacity;
    return h;
}

static void rh_swap(RelaxedHeap *h, int i, int j) {
    RHNode tmp  = h->data[i];
    h->data[i]  = h->data[j];
    h->data[j]  = tmp;
}

static void rh_sift_up(RelaxedHeap *h, int i) {
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->data[parent].key > h->data[i].key) {
            rh_swap(h, parent, i);
            i = parent;
        } else break;
    }
}

static void rh_sift_down(RelaxedHeap *h, int i) {
    while (1) {
        int left  = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;
        if (left  < h->size && h->data[left].key  < h->data[smallest].key) smallest = left;
        if (right < h->size && h->data[right].key < h->data[smallest].key) smallest = right;
        if (smallest == i) break;
        rh_swap(h, i, smallest);
        i = smallest;
    }
}

static void rh_insert(RelaxedHeap *h, int id, double key) {
    if (h->size >= h->capacity) {
        h->capacity *= 2;
        h->data = (RHNode *)realloc(h->data, h->capacity * sizeof(RHNode));
    }
    h->data[h->size].id      = id;
    h->data[h->size].key     = key;
    h->data[h->size].deleted = 0;
    rh_sift_up(h, h->size);
    h->size++;
}

/* lazy delete: mark top as deleted, sift replacement down */
static int rh_extract_min(RelaxedHeap *h, int *id_out, double *key_out) {
    /* skip lazily deleted entries */
    while (h->size > 0 && h->data[0].deleted) {
        h->data[0] = h->data[--h->size];
        if (h->size > 0) rh_sift_down(h, 0);
    }
    if (h->size == 0) return 0;

    *id_out  = h->data[0].id;
    *key_out = h->data[0].key;

    h->data[0] = h->data[--h->size];
    if (h->size > 0) rh_sift_down(h, 0);
    return 1;
}

static void rh_destroy(RelaxedHeap *h) {
    free(h->data);
    free(h);
}

/* ── Benchmark ────────────────────────────────────────────────────────────── */
int main(void) {
    int sizes[]   = {10000, 30000, 100000, 300000, 1000000, 3000000, 10000000};
    int num_sizes = 7;

    printf("B5 Relaxed Heap Baseline\n");
    printf("%-12s %-12s %-12s\n", "n", "time(s)", "ops");

    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        srand(42);

        RelaxedHeap *h = rh_create(n);
        double t0 = get_time();

        for (int i = 0; i < n; i++) {
            double key = (double)rand() / RAND_MAX * 1000.0;
            rh_insert(h, i, key);
        }

        int ops = 0;
        for (int i = 0; i < n / 2; i++) {
            int id; double key;
            if (!rh_extract_min(h, &id, &key)) break;
            ops++;
        }

        double elapsed = get_time() - t0;
        printf("%-12d %-12.4f %-12d\n", n, elapsed, ops);
        fflush(stdout);

        rh_destroy(h);
    }
    return 0;
}