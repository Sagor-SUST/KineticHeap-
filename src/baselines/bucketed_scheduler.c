#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

static double get_time(void) {
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (double)cnt.QuadPart / (double)freq.QuadPart;
}

typedef struct { double key; int id; } Item;

typedef struct {
    Item *data;
    int   size, cap;
} Heap;

Heap *heap_create(int cap) {
    Heap *h = malloc(sizeof(Heap));
    h->data = malloc(cap * sizeof(Item));
    h->size = 0; h->cap = cap;
    return h;
}

void heap_push(Heap *h, int id, double key) {
    int i = h->size++;
    h->data[i] = (Item){key, id};
    while (i > 0) {
        int p = (i-1)/2;
        if (h->data[p].key <= h->data[i].key) break;
        Item t = h->data[p]; h->data[p] = h->data[i]; h->data[i] = t;
        i = p;
    }
}

Item heap_pop(Heap *h) {
    Item top = h->data[0];
    h->data[0] = h->data[--h->size];
    int i = 0;
    while (1) {
        int l=2*i+1, r=2*i+2, m=i;
        if (l < h->size && h->data[l].key < h->data[m].key) m=l;
        if (r < h->size && h->data[r].key < h->data[m].key) m=r;
        if (m==i) break;
        Item t = h->data[m]; h->data[m] = h->data[i]; h->data[i] = t;
        i = m;
    }
    return top;
}

int main(void) {
    int sizes[] = {10000,30000,100000,300000,1000000,3000000,10000000};
    int nsizes = 7;

    printf("n,time_sec,certificates\n");
    fflush(stdout);

    for (int si = 0; si < nsizes; si++) {
        int n = sizes[si];
        srand(42);

        Heap *h = heap_create(n * 2 + 16);
        for (int i = 0; i < n; i++) {
            double a = (double)rand() / RAND_MAX * 1000.0;
            heap_push(h, i, a);
        }

        int    certs = 0;
        double t0    = get_time();

        while (certs < n * 10 && h->size > 0) {
            Item it = heap_pop(h);
            double nk = it.key + 0.5 + ((double)rand()/RAND_MAX)*0.5;
            heap_push(h, it.id, nk);
            certs++;
        }

        double elapsed = get_time() - t0;
        printf("%d,%.4f,%d\n", n, elapsed, certs);
        fflush(stdout);

        free(h->data); free(h);
    }
    return 0;
}