#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static double get_time(void) {
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (double)cnt.QuadPart / (double)freq.QuadPart;
}

/* Simple min-heap based calendar queue simulation */
typedef struct {
    double *keys;
    int    *ids;
    int     size;
    int     cap;
} MinHeap;

MinHeap *mh_create(int cap) {
    MinHeap *h = malloc(sizeof(MinHeap));
    h->keys = malloc(cap * sizeof(double));
    h->ids  = malloc(cap * sizeof(int));
    h->size = 0;
    h->cap  = cap;
    return h;
}

void mh_push(MinHeap *h, int id, double key) {
    int i = h->size++;
    h->keys[i] = key;
    h->ids[i]  = id;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h->keys[p] <= h->keys[i]) break;
        double tk = h->keys[p]; h->keys[p] = h->keys[i]; h->keys[i] = tk;
        int    ti = h->ids[p];  h->ids[p]  = h->ids[i];  h->ids[i]  = ti;
        i = p;
    }
}

void mh_pop(MinHeap *h, int *id, double *key) {
    *key = h->keys[0];
    *id  = h->ids[0];
    h->size--;
    h->keys[0] = h->keys[h->size];
    h->ids[0]  = h->ids[h->size];
    int i = 0;
    while (1) {
        int l = 2*i+1, r = 2*i+2, m = i;
        if (l < h->size && h->keys[l] < h->keys[m]) m = l;
        if (r < h->size && h->keys[r] < h->keys[m]) m = r;
        if (m == i) break;
        double tk = h->keys[m]; h->keys[m] = h->keys[i]; h->keys[i] = tk;
        int    ti = h->ids[m];  h->ids[m]  = h->ids[i];  h->ids[i]  = ti;
        i = m;
    }
}

int main(void) {
    int sizes[] = {10000, 30000, 100000, 300000, 1000000, 3000000, 10000000};
    int nsizes = 7;

    printf("n,time_sec,certificates\n");
    fflush(stdout);

    for (int si = 0; si < nsizes; si++) {
        int n = sizes[si];
        srand(42);

        MinHeap *h = mh_create(n * 2);
        for (int i = 0; i < n; i++) {
            double a = (double)rand() / RAND_MAX * 1000.0;
            mh_push(h, i, a);
        }

        int    certs = 0;
        double t0    = get_time();

        while (certs < n * 10 && h->size > 0) {
            int id; double key;
            mh_pop(h, &id, &key);
            double new_key = key + 0.5 + ((double)rand()/RAND_MAX)*0.5;
            mh_push(h, id, new_key);
            certs++;
        }

        double elapsed = get_time() - t0;
        printf("%d,%.4f,%d\n", n, elapsed, certs);
        fflush(stdout);

        free(h->keys); free(h->ids); free(h);
    }
    return 0;
}