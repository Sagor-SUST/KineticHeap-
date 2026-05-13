#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define TRIALS 30
#define MAX_N 10100000

typedef struct PNode {
    double key;
    int id;
    struct PNode *child, *sibling;
} PNode;

static PNode pool[MAX_N];
static int pool_idx = 0;
static PNode *arr[MAX_N];  /* global — stack overflow এড়াতে */

static PNode *ph_merge(PNode *a, PNode *b) {
    if (!a) return b;
    if (!b) return a;
    if (a->key <= b->key) {
        b->sibling = a->child;
        a->child = b;
        return a;
    }
    a->sibling = b->child;
    b->child = a;
    return b;
}

static PNode *ph_insert(PNode *root, double key, int id) {
    PNode *n = &pool[pool_idx++];
    n->key = key; n->id = id;
    n->child = n->sibling = NULL;
    return ph_merge(root, n);
}

static PNode *ph_merge_pairs_iter(PNode *n) {
    if (!n || !n->sibling) return n;
    int cnt = 0;
    while (n) {
        arr[cnt++] = n;
        PNode *next = n->sibling;
        n->sibling = NULL;
        n = next;
    }
    int i = 0;
    while (i + 1 < cnt) {
        arr[i/2] = ph_merge(arr[i], arr[i+1]);
        i += 2;
    }
    int half = cnt / 2;
    if (cnt % 2 == 1) arr[half] = arr[cnt-1], half++;
    for (int j = half - 1; j > 0; j--)
        arr[j-1] = ph_merge(arr[j-1], arr[j]);
    return arr[0];
}

static PNode *ph_delete_min(PNode *root) {
    if (!root) return NULL;
    return ph_merge_pairs_iter(root->child);
}

static double elapsed_sec(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;
}

int main(void) {
    int sizes[] = {10000, 30000, 100000, 300000, 1000000, 3000000, 10000000};
    int ns = 7;
    double t95 = 2.045;

    FILE *fp = fopen("results/pairing_stats.csv", "w");
    if (!fp) { fprintf(stderr, "Cannot open results/pairing_stats.csv\n"); return 1; }
    fprintf(fp, "n,mean_time,sd_time,ci95_lo,ci95_hi\n");

    printf("=== PAIRING HEAP BASELINE (30 trials, 95%% CI) ===\n\n");

    for (int s = 0; s < ns; s++) {
        int n = sizes[s];
        printf("Testing n=%d (30 trials)...\n", n); fflush(stdout);

        double times[TRIALS];

        for (int trial = 0; trial < TRIALS; trial++) {
            srand(42 + trial * 1000);
            pool_idx = 0;
            PNode *root = NULL;

            for (int i = 0; i < n; i++) {
                double a = (double)rand() / RAND_MAX * 1000.0;
                root = ph_insert(root, a, i);
            }

            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (int i = 0; i < n; i++)
                root = ph_delete_min(root);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            times[trial] = elapsed_sec(t0, t1);
        }

        double mean = 0;
        for (int t = 0; t < TRIALS; t++) mean += times[t];
        mean /= TRIALS;
        double var = 0;
        for (int t = 0; t < TRIALS; t++) var += (times[t]-mean)*(times[t]-mean);
        double sd = sqrt(var/(TRIALS-1));
        double ci = t95 * sd / sqrt(TRIALS);

        printf("  n=%-9d | mean=%.4fs | SD=%.4f | 95%%CI=[%.4f, %.4f]\n",
               n, mean, sd, mean-ci, mean+ci);
        fflush(stdout);
        fprintf(fp, "%d,%.6f,%.6f,%.6f,%.6f\n", n, mean, sd, mean-ci, mean+ci);
    }

    fclose(fp);
    printf("\nDone! Saved to results/pairing_stats.csv\n");
    return 0;
}
