#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* ── B2 Fibonacci-Heap Kinetic baseline (eager evaluation, 30 trials, 95% CI) ──
   Mirrors the original bench_scale.c logic but with statistical rigor.
   Static keys, extract-min only — same methodology as binaryheap/pairing benches.
   NOTE: B2 uses eager mode → every step re-evaluates all keys → slow at large n. */

#define TRIALS    30
#define MAX_N  11000000

/* ── Minimal Fibonacci Heap (same as original B2 implementation) ── */
typedef struct FHNode {
    double key;
    int degree;
    int mark;
    struct FHNode *parent, *child, *left, *right;
} FHNode;

typedef struct { FHNode *min; int n; } FibHeap;

static FHNode pool[MAX_N];
static int pool_idx;

static FHNode *fh_new_node(double key) {
    FHNode *x = &pool[pool_idx++];
    x->key = key; x->degree = 0; x->mark = 0;
    x->parent = x->child = NULL;
    x->left = x->right = x;
    return x;
}

static void fh_link(FibHeap *h, FHNode *a, FHNode *b) {
    /* remove a from root list, make a child of b */
    a->left->right = a->right;
    a->right->left = a->left;
    a->parent = b;
    if (!b->child) { b->child = a; a->left = a->right = a; }
    else {
        a->right = b->child;
        a->left  = b->child->left;
        b->child->left->right = a;
        b->child->left = a;
    }
    b->degree++;
    a->mark = 0;
}

static void fh_consolidate(FibHeap *h) {
    int max_deg = (int)(log2(h->n) + 2) + 2;
    FHNode **A = calloc(max_deg + 1, sizeof(FHNode*));
    /* collect root list */
    int cnt = 0; FHNode *x = h->min;
    do { cnt++; x = x->right; } while (x != h->min);
    FHNode **roots = malloc(cnt * sizeof(FHNode*));
    x = h->min;
    for (int i = 0; i < cnt; i++) { roots[i] = x; x = x->right; }
    for (int i = 0; i < cnt; i++) {
        FHNode *w = roots[i]; int d = w->degree;
        while (d <= max_deg && A[d]) {
            FHNode *y = A[d];
            if (w->key > y->key) { FHNode *t = w; w = y; y = t; }
            fh_link(h, y, w);
            A[d++] = NULL;
        }
        if (d <= max_deg) A[d] = w;
    }
    h->min = NULL;
    for (int i = 0; i <= max_deg; i++) {
        if (!A[i]) continue;
        if (!h->min) { h->min = A[i]; A[i]->left = A[i]->right = A[i]; }
        else {
            A[i]->right = h->min->right;
            A[i]->left  = h->min;
            h->min->right->left = A[i];
            h->min->right = A[i];
            if (A[i]->key < h->min->key) h->min = A[i];
        }
    }
    free(A); free(roots);
}

static void fh_insert(FibHeap *h, double key) {
    FHNode *x = fh_new_node(key);
    if (!h->min) { h->min = x; }
    else {
        x->right = h->min->right;
        x->left  = h->min;
        h->min->right->left = x;
        h->min->right = x;
        if (key < h->min->key) h->min = x;
    }
    h->n++;
}

static double fh_extract_min(FibHeap *h) {
    FHNode *z = h->min;
    if (!z) return 1e300;
    double ret = z->key;
    /* add children to root list */
    if (z->child) {
        FHNode *c = z->child, *next;
        do {
            next = c->right;
            c->left->right = c->right;
            c->right->left = c->left;
            c->right = h->min->right;
            c->left  = h->min;
            h->min->right->left = c;
            h->min->right = c;
            c->parent = NULL;
            c = next;
        } while (c != z->child);
    }
    z->left->right = z->right;
    z->right->left = z->left;
    if (z == z->right) h->min = NULL;
    else { h->min = z->right; fh_consolidate(h); }
    h->n--;
    return ret;
}

static double run_once(int n, unsigned int *seed) {
    pool_idx = 0;
    FibHeap h = {NULL, 0};
    for (int i = 0; i < n; i++) {
        *seed = *seed * 1664525u + 1013904223u;
        fh_insert(&h, (double)(*seed) / 4294967296.0);
    }
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < n; i++) fh_extract_min(&h);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    return (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) * 1e-9;
}

int main(void) {
    int sizes[] = {10000, 30000, 100000, 300000, 1000000, 3000000, 10000000};
    int ns = 7;
    FILE *fp = fopen("results/b2_stats.csv", "w");
    fprintf(fp, "n,mean,sd,ci_lo,ci_hi\n");
    printf("=== B2 FIB-HEAP KINETIC BASELINE (30 trials, 95%% CI) ===\n");
    for (int s = 0; s < ns; s++) {
        int n = sizes[s];
        double times[TRIALS];
        unsigned int seed = 42;
        printf("Testing n=%d (%d trials)...\n", n, TRIALS);
        for (int t = 0; t < TRIALS; t++) times[t] = run_once(n, &seed);
        double mean = 0;
        for (int t = 0; t < TRIALS; t++) mean += times[t];
        mean /= TRIALS;
        double sd = 0;
        for (int t = 0; t < TRIALS; t++) sd += (times[t]-mean)*(times[t]-mean);
        sd = sqrt(sd / (TRIALS-1));
        double ci = 1.96 * sd / sqrt(TRIALS);
        printf("  n=%-10d | mean=%.4fs | SD=%.4f | 95%%CI=[%.4f, %.4f]\n",
               n, mean, sd, mean-ci, mean+ci);
        fprintf(fp, "%d,%.6f,%.6f,%.6f,%.6f\n", n, mean, sd, mean-ci, mean+ci);
    }
    fclose(fp);
    printf("Done! Saved to results/b2_stats.csv\n");
    return 0;
}
