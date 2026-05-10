#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* B10 - LibKinetic (Simplified)
 * Simulates LibKinetic's C++17 kinetic event scheduler
 * using a priority queue over trajectory crossing times.
 */

static double get_time(void) {
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (double)cnt.QuadPart / (double)freq.QuadPart;
}

typedef struct {
    double key;
    double slope;
    int    id;
    double next_event;
} Particle;

typedef struct {
    double event_time;
    int    pid;
} Event;

typedef struct {
    Event *data;
    int    size, cap;
} EventQueue;

EventQueue *eq_create(int cap) {
    EventQueue *q = malloc(sizeof(EventQueue));
    q->data = malloc(cap * sizeof(Event));
    q->size = 0; q->cap = cap;
    return q;
}

void eq_push(EventQueue *q, double t, int pid) {
    if (q->size >= q->cap) {
        q->cap *= 2;
        q->data = realloc(q->data, q->cap * sizeof(Event));
    }
    int i = q->size++;
    q->data[i] = (Event){t, pid};
    while (i > 0) {
        int p = (i-1)/2;
        if (q->data[p].event_time <= q->data[i].event_time) break;
        Event tmp = q->data[p];
        q->data[p] = q->data[i];
        q->data[i] = tmp;
        i = p;
    }
}

Event eq_pop(EventQueue *q) {
    Event top = q->data[0];
    q->data[0] = q->data[--q->size];
    int i = 0;
    while (1) {
        int l=2*i+1, r=2*i+2, m=i;
        if (l<q->size && q->data[l].event_time < q->data[m].event_time) m=l;
        if (r<q->size && q->data[r].event_time < q->data[m].event_time) m=r;
        if (m==i) break;
        Event tmp = q->data[m];
        q->data[m] = q->data[i];
        q->data[i] = tmp;
        i = m;
    }
    return top;
}

int main(void) {
    int sizes[] = {10000, 30000, 100000, 300000, 1000000, 3000000, 10000000};
    int nsizes = 7;

    printf("n,time_sec,certificates\n");
    fflush(stdout);

    for (int si = 0; si < nsizes; si++) {
        int n = sizes[si];
        srand(42);

        Particle *parts = malloc(n * sizeof(Particle));
        EventQueue *eq  = eq_create(n * 2 + 16);

        for (int i = 0; i < n; i++) {
            parts[i].key        = (double)rand() / RAND_MAX * 1000.0;
            parts[i].slope      = ((double)rand() / RAND_MAX * 2.0) - 1.0;
            parts[i].id         = i;
            parts[i].next_event = parts[i].key +
                                  0.5 + ((double)rand()/RAND_MAX) * 0.5;
            eq_push(eq, parts[i].next_event, i);
        }

        int    certs = 0;
        double cur_t = 0.0;
        double t0    = get_time();

        while (eq->size > 0 && certs < n * 10) {
            Event ev = eq_pop(eq);
            if (ev.event_time > 100.0) break;
            cur_t = ev.event_time;

            int pid = ev.pid;
            /* update particle key and schedule next event */
            parts[pid].key = parts[pid].key +
                             parts[pid].slope * (cur_t - parts[pid].key * 0.001);
            double next = cur_t + 0.5 +
                          ((double)rand()/RAND_MAX) * 0.5;
            if (next < 100.0) {
                parts[pid].next_event = next;
                eq_push(eq, next, pid);
            }
            certs++;
        }

        double elapsed = get_time() - t0;
        printf("%d,%.4f,%d\n", n, elapsed, certs);
        fflush(stdout);

        free(parts);
        free(eq->data);
        free(eq);
    }
    return 0;
}