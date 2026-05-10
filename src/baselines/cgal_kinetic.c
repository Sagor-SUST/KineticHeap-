#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* B9 - CGAL Kinetic Sort (Simplified)
 * Simulates CGAL's kinetic sort approach:
 * maintains sorted order under linear trajectories
 * using certificate-based event processing.
 */

static double get_time(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

typedef struct {
    double a, b;   /* key(t) = a + b*t */
    int    id;
} Element;

typedef struct {
    double event_time;
    int    i, j;   /* adjacent pair that will swap */
} Certificate;

typedef struct {
    Certificate *data;
    int          size, cap;
} CertHeap;

CertHeap *ch_create(int cap) {
    CertHeap *h = malloc(sizeof(CertHeap));
    h->data = malloc(cap * sizeof(Certificate));
    h->size = 0; h->cap = cap;
    return h;
}

void ch_push(CertHeap *h, double t, int i, int j) {
    if (h->size >= h->cap) {
        h->cap *= 2;
        h->data = realloc(h->data, h->cap * sizeof(Certificate));
    }
    int k = h->size++;
    h->data[k] = (Certificate){t, i, j};
    while (k > 0) {
        int p = (k-1)/2;
        if (h->data[p].event_time <= h->data[k].event_time) break;
        Certificate tmp = h->data[p];
        h->data[p] = h->data[k];
        h->data[k] = tmp;
        k = p;
    }
}

Certificate ch_pop(CertHeap *h) {
    Certificate top = h->data[0];
    h->data[0] = h->data[--h->size];
    int k = 0;
    while (1) {
        int l=2*k+1, r=2*k+2, m=k;
        if (l<h->size && h->data[l].event_time < h->data[m].event_time) m=l;
        if (r<h->size && h->data[r].event_time < h->data[m].event_time) m=r;
        if (m==k) break;
        Certificate tmp = h->data[m];
        h->data[m] = h->data[k];
        h->data[k] = tmp;
        k = m;
    }
    return top;
}

static double crossing_time(Element *a, Element *b) {
    /* a.a + a.b*t = b.a + b.b*t => t = (b.a - a.a)/(a.b - b.b) */
    double db = a->b - b->b;
    if (fabs(db) < 1e-12) return 1e18;
    double t = (b->a - a->a) / db;
    return t;
}

int main(void) {
    int sizes[] = {10000, 30000, 100000, 300000, 1000000, 3000000, 10000000};
    int nsizes = 7;

    printf("n,time_sec,certificates\n");
    fflush(stdout);

    for (int si = 0; si < nsizes; si++) {
        int n = sizes[si];
        srand(42);

        Element *elems = malloc(n * sizeof(Element));
        for (int i = 0; i < n; i++) {
            elems[i].a  = (double)rand() / RAND_MAX * 1000.0;
            elems[i].b  = ((double)rand() / RAND_MAX * 2.0) - 1.0;
            elems[i].id = i;
        }

        /* build initial certificate heap for adjacent pairs */
        CertHeap *ch = ch_create(n * 2 + 16);
        for (int i = 0; i < n-1; i++) {
            double t = crossing_time(&elems[i], &elems[i+1]);
            if (t > 0 && t < 100.0)
                ch_push(ch, t, i, i+1);
        }

        int    certs = 0;
        double cur_t = 0.0;
        double t0    = get_time();

        while (ch->size > 0 && certs < n * 10) {
            Certificate ev = ch_pop(ch);
            if (ev.event_time > 100.0) break;
            cur_t = ev.event_time;

            int i = ev.i, j = ev.j;
            if (i < 0 || j >= n) continue;

            /* swap adjacent elements */
            Element tmp = elems[i];
            elems[i] = elems[j];
            elems[j] = tmp;

            /* re-certify neighbors */
            if (i > 0) {
                double t = crossing_time(&elems[i-1], &elems[i]);
                if (t > cur_t && t < 100.0)
                    ch_push(ch, t, i-1, i);
            }
            if (j < n-1) {
                double t = crossing_time(&elems[j], &elems[j+1]);
                if (t > cur_t && t < 100.0)
                    ch_push(ch, t, j, j+1);
            }
            double t2 = crossing_time(&elems[i], &elems[j]);
            if (t2 > cur_t && t2 < 100.0)
                ch_push(ch, t2, i, j);

            certs++;
        }

        double elapsed = get_time() - t0;
        printf("%d,%.4f,%d\n", n, elapsed, certs);
        fflush(stdout);

        free(elems);
        free(ch->data);
        free(ch);
    }
    return 0;
}