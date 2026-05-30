#include "../include/kineticheap.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

FibHeap *fh_create(void) {
    FibHeap *fh = (FibHeap *)malloc(sizeof(FibHeap));
    if (!fh) { fprintf(stderr, "fh_create: malloc failed\n"); exit(1); }
    fh->min  = NULL;
    fh->size = 0;
    return fh;
}

static void fh_link_root(FibHeap *fh, FHNode *node) {
    if (!fh->min) {
        node->left  = node;
        node->right = node;
        fh->min     = node;
    } else {
        node->right              = fh->min;
        node->left               = fh->min->left;
        fh->min->left->right     = node;
        fh->min->left            = node;
        if (node->key < fh->min->key)
            fh->min = node;
    }
}

FHNode *fh_insert(FibHeap *fh, double key, int i, int j) {
    FHNode *node = (FHNode *)malloc(sizeof(FHNode));
    if (!node) { fprintf(stderr, "fh_insert: malloc failed\n"); exit(1); }
    node->key    = key;
    node->heap_i = i;
    node->heap_j = j;
    node->degree = 0;
    node->mark   = 0;
    node->parent = NULL;
    node->child  = NULL;
    node->left   = node;
    node->right  = node;
    fh_link_root(fh, node);
    fh->size++;
    return node;
}

static void fh_make_child(FHNode *y, FHNode *x) {
    y->left->right = y->right;
    y->right->left = y->left;
    y->parent = x;
    if (!x->child) {
        x->child = y;
        y->left  = y;
        y->right = y;
    } else {
        y->right              = x->child;
        y->left               = x->child->left;
        x->child->left->right = y;
        x->child->left        = y;
    }
    x->degree++;
    y->mark = 0;
}

static void fh_consolidate(FibHeap *fh) {
    int max_degree = 64;
    FHNode **A = (FHNode **)calloc(max_degree, sizeof(FHNode *));
    if (!A) { fprintf(stderr, "fh_consolidate: calloc failed\n"); exit(1); }

    FHNode *roots[4096];
    int     nroots = 0;
    FHNode *cur = fh->min;
    if (cur) {
        do {
            roots[nroots++] = cur;
            cur = cur->right;
        } while (cur != fh->min && nroots < 4096);
    }

    for (int k = 0; k < nroots; k++) {
        FHNode *x = roots[k];
        int d = x->degree;
        while (A[d]) {
            FHNode *y = A[d];
            if (x->key > y->key) { FHNode *tmp = x; x = y; y = tmp; }
            fh_make_child(y, x);
            A[d] = NULL;
            d++;
        }
        A[d] = x;
    }

    fh->min = NULL;
    for (int d = 0; d < max_degree; d++) {
        if (A[d]) fh_link_root(fh, A[d]);
    }
    free(A);
}

FHNode *fh_extract_min(FibHeap *fh) {
    FHNode *z = fh->min;
    if (!z) return NULL;

    if (z->child) {
        FHNode *child = z->child;
        do {
            FHNode *next  = child->right;
            child->parent = NULL;
            fh_link_root(fh, child);
            child = next;
        } while (child != z->child);
    }

    z->left->right = z->right;
    z->right->left = z->left;
    if (z == z->right) {
        fh->min = NULL;
    } else {
        fh->min = z->right;
        fh_consolidate(fh);
    }
    fh->size--;
    return z;
}

static void fh_cut(FibHeap *fh, FHNode *x, FHNode *y) {
    if (x->right == x) {
        y->child = NULL;
    } else {
        x->left->right = x->right;
        x->right->left = x->left;
        if (y->child == x) y->child = x->right;
    }
    y->degree--;
    x->parent = NULL;
    x->mark   = 0;
    fh_link_root(fh, x);
}

static void fh_cascading_cut(FibHeap *fh, FHNode *y) {
    FHNode *z = y->parent;
    if (z) {
        if (!y->mark) { y->mark = 1; }
        else { fh_cut(fh, y, z); fh_cascading_cut(fh, z); }
    }
}

void fh_decrease_key(FibHeap *fh, FHNode *node, double new_key) {
    if (new_key > node->key) return;
    node->key = new_key;
    FHNode *y = node->parent;
    if (y && node->key < y->key) {
        fh_cut(fh, node, y);
        fh_cascading_cut(fh, y);
    }
    if (node->key < fh->min->key) fh->min = node;
}

void fh_delete(FibHeap *fh, FHNode *node) {
    fh_decrease_key(fh, node, -KH_INF);
    FHNode *extracted = fh_extract_min(fh);
    free(extracted);
}

void fh_destroy(FibHeap *fh) {
    if (!fh) return;
    while (fh->size > 0) {
        FHNode *n = fh_extract_min(fh);
        free(n);
    }
    free(fh);
}

KineticHeap *kh_create(int capacity) {
    KineticHeap *kh = (KineticHeap *)malloc(sizeof(KineticHeap));
    if (!kh) { fprintf(stderr, "kh_create: malloc failed\n"); exit(1); }
    kh->nodes = (HeapNode *)calloc(capacity + 1, sizeof(HeapNode));
    if (!kh->nodes) { fprintf(stderr, "kh_create: nodes malloc failed\n"); exit(1); }
    kh->size     = 0;
    kh->capacity = capacity;
    kh->time     = 0.0;
    kh->fheap    = fh_create();
    return kh;
}

void kh_destroy(KineticHeap *kh) {
    if (!kh) return;
    fh_destroy(kh->fheap);
    free(kh->nodes);
    free(kh);
}

void kh_apply_lazy(KineticHeap *kh, int idx) {
    HeapNode *n    = &kh->nodes[idx];
    n->lazy_offset = n->slope * (kh->time - n->last_sync);
    n->last_sync   = kh->time;  /* mark as synced to current time */
}

double kh_eval_key(KineticHeap *kh, int idx) {
    kh_apply_lazy(kh, idx);     /* apply lazy offset before reading key */
    HeapNode *n = &kh->nodes[idx];
    return n->intercept + n->lazy_offset;
}

double kh_cert_time(KineticHeap *kh, int p, int c) {
    double ai = kh->nodes[p].intercept, bi = kh->nodes[p].slope;
    double aj = kh->nodes[c].intercept, bj = kh->nodes[c].slope;
    double db = bi - bj;
    if (fabs(db) < KH_EPSILON) return KH_INF;
    double t = (aj - ai) / db;
    return (t > kh->time) ? t : KH_INF;
}

void kh_issue_cert(KineticHeap *kh, int p, int c) {
    double t = kh_cert_time(kh, p, c);
    if (t < KH_INF) {
        FHNode *fn         = fh_insert(kh->fheap, t, p, c);
        kh->nodes[c].cert  = fn;
    } else {
        kh->nodes[c].cert  = NULL;
    }
}

void kh_revoke_cert(KineticHeap *kh, int p, int c) {
    (void)p;
    FHNode *fn = kh->nodes[c].cert;
    if (fn) {
        fh_delete(kh->fheap, fn);
        kh->nodes[c].cert = NULL;
    }
}

static void kh_swap(KineticHeap *kh, int i, int j) {
    HeapNode tmp     = kh->nodes[i];
    kh->nodes[i]     = kh->nodes[j];
    kh->nodes[j]     = tmp;
    kh->nodes[i].pos = i;
    kh->nodes[j].pos = j;
}

int kh_sift_up(KineticHeap *kh, int idx) {
    while (idx > 1) {
        int p = idx / 2;
        if (kh_eval_key(kh, p) > kh_eval_key(kh, idx)) {
            kh_revoke_cert(kh, p, idx);
            if (p > 1) kh_revoke_cert(kh, p / 2, p);
            kh_swap(kh, p, idx);
            kh_issue_cert(kh, p, idx);
            if (p > 1) kh_issue_cert(kh, p / 2, p);
            idx = p;
        } else break;
    }
    return idx;  /* return final position after sifting */
}

void kh_sift_down(KineticHeap *kh, int idx) {
    int n = kh->size;
    while (1) {
        int smallest = idx;
        int l = 2 * idx, r = 2 * idx + 1;
        if (l <= n && kh_eval_key(kh, l) < kh_eval_key(kh, smallest))
            smallest = l;
        if (r <= n && kh_eval_key(kh, r) < kh_eval_key(kh, smallest))
            smallest = r;
        if (smallest == idx) break;

        if (2 * idx     <= n) kh_revoke_cert(kh, idx, 2 * idx);
        if (2 * idx + 1 <= n) kh_revoke_cert(kh, idx, 2 * idx + 1);

        kh_swap(kh, idx, smallest);

        if (2 * idx     <= n) kh_issue_cert(kh, idx, 2 * idx);
        if (2 * idx + 1 <= n) kh_issue_cert(kh, idx, 2 * idx + 1);

        idx = smallest;
    }
}

void kh_insert(KineticHeap *kh, double intercept, double slope, int id) {
    if (kh->size >= kh->capacity) {
        fprintf(stderr, "kh_insert: heap full\n");
        return;
    }
    kh->size++;
    int idx        = kh->size;
    HeapNode *n    = &kh->nodes[idx];
    n->intercept   = intercept;
    n->slope       = slope;
    n->last_sync   = kh->time;
    n->lazy_offset = 0.0;
    n->pos         = idx;
    n->id          = id;
    n->cert        = NULL;
    int final_pos = kh_sift_up(kh, idx);
    /* issue cert only if node did not move (sift_up issues certs on each swap) */
    if (final_pos == idx && idx > 1)
        kh_issue_cert(kh, idx / 2, idx);
}

HeapNode kh_delete_min(KineticHeap *kh) {
    HeapNode result = kh->nodes[1];
    int n = kh->size;
    if (2 <= n) kh_revoke_cert(kh, 1, 2);
    if (3 <= n) kh_revoke_cert(kh, 1, 3);
    kh_swap(kh, 1, n);
    kh->size--;
    if (kh->size > 0) {
        kh->nodes[1].cert = NULL;
        kh_sift_down(kh, 1);
        if (2 <= kh->size) kh_issue_cert(kh, 1, 2);
        if (3 <= kh->size) kh_issue_cert(kh, 1, 3);
    }
    return result;
}

void kh_handle_event(KineticHeap *kh) {
    if (!kh->fheap->min) return;
    FHNode *ev = fh_extract_min(kh->fheap);
    if (!ev) return;

    double t_ev = ev->key;
    int    i    = ev->heap_i;
    int    j    = ev->heap_j;
    free(ev);

    /* validate indices */
    if (i < 1 || i > kh->size) return;
    if (j < 1 || j > kh->size) return;
    if (j != i * 2 && j != i * 2 + 1) return; /* stale event */

    kh->time = t_ev;

    /* revoke affected certificates */
    kh->nodes[j].cert = NULL; /* already extracted */
    if (2*j   <= kh->size && kh->nodes[2*j].cert)
        kh_revoke_cert(kh, j, 2*j);
    if (2*j+1 <= kh->size && kh->nodes[2*j+1].cert)
        kh_revoke_cert(kh, j, 2*j+1);

    /* swap i and j */
    kh_swap(kh, i, j);

    /* reissue certificates */
    kh_issue_cert(kh, i, j);
    if (2*i   <= kh->size) kh_issue_cert(kh, i, 2*i);
    if (2*i+1 <= kh->size) kh_issue_cert(kh, i, 2*i+1);
    if (2*j   <= kh->size) kh_issue_cert(kh, j, 2*j);
    if (2*j+1 <= kh->size) kh_issue_cert(kh, j, 2*j+1);
    if (i > 1) kh_issue_cert(kh, i/2, i);
}

double kh_min_key(KineticHeap *kh) {
    if (kh->size == 0) return KH_INF;
    return kh_eval_key(kh, 1);
}

int kh_size(KineticHeap *kh) {
    return kh->size;
}