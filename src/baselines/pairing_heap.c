/*
 * B3 Baseline: Pairing Heap Priority Queue
 * KineticHeap++ paper — baseline comparison
 * Windows + Linux compatible timing
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32

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

typedef struct PHNode {
    int    id;
    double key;
    struct PHNode *child;
    struct PHNode *sibling;
} PHNode;

static PHNode *ph_new_node(int id, double key) {
    PHNode *n  = (PHNode *)malloc(sizeof(PHNode));
    n->id      = id;
    n->key     = key;
    n->child   = NULL;
    n->sibling = NULL;
    return n;
}

static PHNode *ph_merge(PHNode *a, PHNode *b) {
    if (!a) return b;
    if (!b) return a;
    if (a->key <= b->key) {
        b->sibling = a->child;
        a->child   = b;
        return a;
    } else {
        a->sibling = b->child;
        b->child   = a;
        return b;
    }
}

static PHNode *ph_insert(PHNode *heap, int id, double key) {
    return ph_merge(heap, ph_new_node(id, key));
}

/* two-pass merge using dynamic array */
static PHNode *ph_merge_children(PHNode *first) {
    if (!first || !first->sibling) return first;

    /* count siblings */
    int count = 0;
    PHNode *cur = first;
    while (cur) { count++; cur = cur->sibling; }

    PHNode **pairs = (PHNode **)malloc(count * sizeof(PHNode *));
    int idx = 0;

    /* first pass: pair up */
    cur = first;
    while (cur && cur->sibling) {
        PHNode *a  = cur;
        PHNode *b  = cur->sibling;
        cur        = b->sibling;
        a->sibling = NULL;
        b->sibling = NULL;
        pairs[idx++] = ph_merge(a, b);
    }
    if (cur) pairs[idx++] = cur;

    /* second pass: merge right to left */
    PHNode *result = pairs[--idx];
    while (idx > 0) result = ph_merge(pairs[--idx], result);

    free(pairs);
    return result;
}

static PHNode *ph_extract_min(PHNode *heap) {
    if (!heap) return NULL;
    return ph_merge_children(heap->child);
}

static void ph_free(PHNode *n) {
    if (!n) return;
    ph_free(n->child);
    ph_free(n->sibling);
    free(n);
}

int main(void) {
    int sizes[]   = {10000, 30000, 100000, 300000, 1000000, 3000000, 10000000};
    int num_sizes = 7;

    printf("B3 Pairing Heap Baseline\n");
    printf("%-12s %-12s %-12s\n", "n", "time(s)", "ops");

    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        srand(42);

        double t0    = get_time();
        PHNode *heap = NULL;

        for (int i = 0; i < n; i++) {
            double key = (double)rand() / RAND_MAX * 1000.0;
            heap = ph_insert(heap, i, key);
        }

        int ops = 0;
        for (int i = 0; i < n / 2; i++) {
            if (!heap) break;
            PHNode *old = heap;
            heap = ph_extract_min(heap);
            free(old);
            ops++;
        }

        double elapsed = get_time() - t0;
        printf("%-12d %-12.4f %-12d\n", n, elapsed, ops);
        fflush(stdout);

        ph_free(heap);
    }
    return 0;
}