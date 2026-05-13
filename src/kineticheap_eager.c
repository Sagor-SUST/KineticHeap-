/*
 * kineticheap_eager.c  —  V0 baseline: NO LPE, pointer-style eager evaluation
 * Every key is evaluated eagerly on every comparison.
 * This is the B2-equivalent baseline for the ablation study.
 */
#include "../include/kineticheap.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

/* ── Fibonacci heap (identical to main implementation) ── */
FibHeap *fh_create_eager(void) {
    FibHeap *fh = (FibHeap *)malloc(sizeof(FibHeap));
    fh->min = NULL; fh->size = 0;
    return fh;
}

static void fh_link_root_e(FibHeap *fh, FHNode *node) {
    if (!fh->min) {
        node->left = node; node->right = node; fh->min = node;
    } else {
        node->right = fh->min; node->left = fh->min->left;
        fh->min->left->right = node; fh->min->left = node;
        if (node->key < fh->min->key) fh->min = node;
    }
}

FHNode *fh_insert_eager(FibHeap *fh, double key, int i, int j) {
    FHNode *node = (FHNode *)malloc(sizeof(FHNode));
    node->key = key; node->heap_i = i; node->heap_j = j;
    node->degree = 0; node->mark = 0;
    node->parent = NULL; node->child = NULL;
    node->left = node; node->right = node;
    fh_link_root_e(fh, node);
    fh->size++;
    return node;
}

static void fh_make_child_e(FHNode *y, FHNode *x) {
    y->left->right = y->right; y->right->left = y->left;
    y->parent = x;
    if (!x->child) { x->child = y; y->left = y; y->right = y; }
    else {
        y->right = x->child; y->left = x->child->left;
        x->child->left->right = y; x->child->left = y;
    }
    x->degree++; y->mark = 0;
}

static void fh_consolidate_e(FibHeap *fh) {
    int max_degree = 64;
    FHNode **A = (FHNode **)calloc(max_degree, sizeof(FHNode *));
    FHNode *roots[4096]; int nroots = 0;
    FHNode *cur = fh->min;
    if (cur) { do { roots[nroots++] = cur; cur = cur->right; }
               while (cur != fh->min && nroots < 4096); }
    for (int k = 0; k < nroots; k++) {
        FHNode *x = roots[k]; int d = x->degree;
        while (A[d]) {
            FHNode *y = A[d];
            if (x->key > y->key) { FHNode *tmp = x; x = y; y = tmp; }
            fh_make_child_e(y, x); A[d] = NULL; d++;
        }
        A[d] = x;
    }
    fh->min = NULL;
    for (int d = 0; d < max_degree; d++) if (A[d]) fh_link_root_e(fh, A[d]);
    free(A);
}

static FHNode *fh_extract_min_eager(FibHeap *fh) {
    FHNode *z = fh->min; if (!z) return NULL;
    if (z->child) {
        FHNode *child = z->child;
        do { FHNode *next = child->right; child->parent = NULL;
             fh_link_root_e(fh, child); child = next; }
        while (child != z->child);
    }
    z->left->right = z->right; z->right->left = z->left;
    if (z == z->right) fh->min = NULL;
    else { fh->min = z->right; fh_consolidate_e(fh); }
    fh->size--;
    return z;
}

static void fh_cut_e(FibHeap *fh, FHNode *x, FHNode *y) {
    if (x->right == x) y->child = NULL;
    else { x->left->right = x->right; x->right->left = x->left;
           if (y->child == x) y->child = x->right; }
    y->degree--; x->parent = NULL; x->mark = 0;
    fh_link_root_e(fh, x);
}

static void fh_cascading_cut_e(FibHeap *fh, FHNode *y) {
    FHNode *z = y->parent;
    if (z) { if (!y->mark) y->mark = 1;
             else { fh_cut_e(fh, y, z); fh_cascading_cut_e(fh, z); } }
}

static void fh_decrease_key_eager(FibHeap *fh, FHNode *node, double new_key) {
    if (new_key > node->key) return;
    node->key = new_key;
    FHNode *y = node->parent;
    if (y && node->key < y->key) { fh_cut_e(fh, node, y); fh_cascading_cut_e(fh, y); }
    if (node->key < fh->min->key) fh->min = node;
}

static void fh_delete_eager(FibHeap *fh, FHNode *node) {
    fh_decrease_key_eager(fh, node, -KH_INF);
    FHNode *ex = fh_extract_min_eager(fh);
    free(ex);
}

static void fh_destroy_eager(FibHeap *fh) {
    if (!fh) return;
    while (fh->size > 0) { FHNode *n = fh_extract_min_eager(fh); free(n); }
    free(fh);
}

/* ══════════════════════════════════════════════════════
   EAGER KineticHeap — separate struct to avoid conflicts
══════════════════════════════════════════════════════ */
typedef struct {
    HeapNode *nodes;
    int       size;
    int       capacity;
    double    time;
    FibHeap  *fheap;
} EagerHeap;

EagerHeap *eh_create(int capacity) {
    EagerHeap *eh = (EagerHeap *)malloc(sizeof(EagerHeap));
    eh->nodes = (HeapNode *)calloc(capacity + 1, sizeof(HeapNode));
    eh->size = 0; eh->capacity = capacity;
    eh->time = 0.0; eh->fheap = fh_create_eager();
    return eh;
}

void eh_destroy(EagerHeap *eh) {
    if (!eh) return;
    fh_destroy_eager(eh->fheap);
    free(eh->nodes); free(eh);
}

/* EAGER key evaluation — no lazy offset, always recompute */
static double eh_eval_key(EagerHeap *eh, int idx) {
    return eh->nodes[idx].intercept + eh->nodes[idx].slope * eh->time;
}

static double eh_cert_time(EagerHeap *eh, int p, int c) {
    double ai = eh->nodes[p].intercept, bi = eh->nodes[p].slope;
    double aj = eh->nodes[c].intercept, bj = eh->nodes[c].slope;
    double db = bi - bj;
    if (fabs(db) < KH_EPSILON) return KH_INF;
    double t = (aj - ai) / db;
    return (t > eh->time) ? t : KH_INF;
}

static void eh_issue_cert(EagerHeap *eh, int p, int c) {
    double t = eh_cert_time(eh, p, c);
    if (t < KH_INF) {
        FHNode *fn = fh_insert_eager(eh->fheap, t, p, c);
        eh->nodes[c].cert = fn;
    } else {
        eh->nodes[c].cert = NULL;
    }
}

static void eh_revoke_cert(EagerHeap *eh, int p, int c) {
    (void)p;
    FHNode *fn = eh->nodes[c].cert;
    if (fn) { fh_delete_eager(eh->fheap, fn); eh->nodes[c].cert = NULL; }
}

static void eh_swap(EagerHeap *eh, int i, int j) {
    HeapNode tmp = eh->nodes[i];
    eh->nodes[i] = eh->nodes[j];
    eh->nodes[j] = tmp;
    eh->nodes[i].pos = i;
    eh->nodes[j].pos = j;
}

static void eh_sift_up(EagerHeap *eh, int idx) {
    while (idx > 1) {
        int p = idx / 2;
        if (eh_eval_key(eh, p) > eh_eval_key(eh, idx)) {
            eh_revoke_cert(eh, p, idx);
            if (p > 1) eh_revoke_cert(eh, p/2, p);
            eh_swap(eh, p, idx);
            eh_issue_cert(eh, p, idx);
            if (p > 1) eh_issue_cert(eh, p/2, p);
            idx = p;
        } else break;
    }
}

static void eh_sift_down(EagerHeap *eh, int idx) {
    int n = eh->size;
    while (1) {
        int smallest = idx;
        int l = 2*idx, r = 2*idx+1;
        if (l <= n && eh_eval_key(eh, l) < eh_eval_key(eh, smallest)) smallest = l;
        if (r <= n && eh_eval_key(eh, r) < eh_eval_key(eh, smallest)) smallest = r;
        if (smallest == idx) break;
        if (2*idx   <= n) eh_revoke_cert(eh, idx, 2*idx);
        if (2*idx+1 <= n) eh_revoke_cert(eh, idx, 2*idx+1);
        eh_swap(eh, idx, smallest);
        if (2*idx   <= n) eh_issue_cert(eh, idx, 2*idx);
        if (2*idx+1 <= n) eh_issue_cert(eh, idx, 2*idx+1);
        idx = smallest;
    }
}

void eh_insert(EagerHeap *eh, double intercept, double slope, int id) {
    if (eh->size >= eh->capacity) return;
    eh->size++;
    int idx = eh->size;
    HeapNode *n = &eh->nodes[idx];
    n->intercept = intercept; n->slope = slope;
    n->last_sync = eh->time; n->lazy_offset = 0.0;
    n->pos = idx; n->id = id; n->cert = NULL;
    eh_sift_up(eh, idx);
    if (idx > 1) eh_issue_cert(eh, idx/2, idx);
}

void eh_handle_event(EagerHeap *eh) {
    if (!eh->fheap->min) return;
    FHNode *ev = fh_extract_min_eager(eh->fheap);
    if (!ev) return;
    double t_ev = ev->key;
    int i = ev->heap_i, j = ev->heap_j;
    free(ev);
    if (i < 1 || i > eh->size) return;
    if (j < 1 || j > eh->size) return;
    if (j != i*2 && j != i*2+1) return;

    eh->time = t_ev;


    eh->nodes[j].cert = NULL;
    if (2*j   <= eh->size && eh->nodes[2*j].cert)   eh_revoke_cert(eh, j, 2*j);
    if (2*j+1 <= eh->size && eh->nodes[2*j+1].cert) eh_revoke_cert(eh, j, 2*j+1);

    eh_swap(eh, i, j);

    eh_issue_cert(eh, i, j);
    if (2*i   <= eh->size) eh_issue_cert(eh, i, 2*i);
    if (2*i+1 <= eh->size) eh_issue_cert(eh, i, 2*i+1);
    if (2*j   <= eh->size) eh_issue_cert(eh, j, 2*j);
    if (2*j+1 <= eh->size) eh_issue_cert(eh, j, 2*j+1);
    if (i > 1) eh_issue_cert(eh, i/2, i);
}

/* Public API for bench */
EagerHeap *eager_create(int cap)  { return eh_create(cap); }
void       eager_destroy(EagerHeap *eh) { eh_destroy(eh); }
void       eager_insert(EagerHeap *eh, double a, double b, int id) { eh_insert(eh,a,b,id); }
void       eager_handle_event(EagerHeap *eh) { eh_handle_event(eh); }
int        eager_has_event(EagerHeap *eh) { return eh->fheap->min != NULL; }
double     eager_min_event_time(EagerHeap *eh) {
    return eh->fheap->min ? eh->fheap->min->key : KH_INF;
}
