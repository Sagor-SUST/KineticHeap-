#ifndef KINETICHEAP_H
#define KINETICHEAP_H

#include <stddef.h>
#include <float.h>

/* ─────────────────────────────────────────
   Constants
───────────────────────────────────────── */
#define KH_MAX_NODES   (1 << 24)   /* 16 million max elements  */
#define KH_INF          DBL_MAX     /* infinity for certificates */
#define KH_EPSILON      1e-12       /* floating-point tolerance  */

/* ─────────────────────────────────────────
   Fibonacci Heap Node (Event Queue)
───────────────────────────────────────── */
typedef struct FHNode {
    double        key;        /* certificate expiry time t_ij  */
    int           heap_i;     /* heap index of element i        */
    int           heap_j;     /* heap index of element j        */
    int           degree;
    int           mark;
    struct FHNode *parent;
    struct FHNode *child;
    struct FHNode *left;
    struct FHNode *right;
} FHNode;

/* ─────────────────────────────────────────
   Fibonacci Heap (used as Event Queue)
───────────────────────────────────────── */
typedef struct {
    FHNode *min;
    int     size;
} FibHeap;

/* ─────────────────────────────────────────
   Heap Node (Interleaved SoA layout)
   Each node stores trajectory ki(t)=ai+bi*t
───────────────────────────────────────── */
typedef struct {
    double  intercept;    /* ai                          */
    double  slope;        /* bi                          */
    double  last_sync;    /* t_last: last time key synced */
    double  lazy_offset;  /* delta_i = bi*(t - t_last)   */
    int     pos;          /* position in heap array       */
    int     id;           /* original item id             */
    FHNode *cert;         /* pointer to certificate in Q  */
} HeapNode;

/* ─────────────────────────────────────────
   KineticHeap — main structure
───────────────────────────────────────── */
typedef struct {
    HeapNode  *nodes;     /* interleaved array, 1-indexed */
    int        size;      /* current number of elements   */
    int        capacity;  /* allocated capacity           */
    double     time;      /* current global time t        */
    FibHeap   *fheap;     /* Fibonacci heap event queue Q */
} KineticHeap;

/* ─────────────────────────────────────────
   Fibonacci Heap API
───────────────────────────────────────── */
FibHeap  *fh_create(void);
void      fh_destroy(FibHeap *fh);
FHNode   *fh_insert(FibHeap *fh, double key, int i, int j);
FHNode   *fh_extract_min(FibHeap *fh);
void      fh_decrease_key(FibHeap *fh, FHNode *node, double new_key);
void      fh_delete(FibHeap *fh, FHNode *node);

/* ─────────────────────────────────────────
   KineticHeap API
───────────────────────────────────────── */
KineticHeap *kh_create(int capacity);
void         kh_destroy(KineticHeap *kh);
void         kh_insert(KineticHeap *kh, double intercept,
                        double slope, int id);
HeapNode     kh_delete_min(KineticHeap *kh);
void         kh_handle_event(KineticHeap *kh);
double       kh_min_key(KineticHeap *kh);
int          kh_size(KineticHeap *kh);

/* ─────────────────────────────────────────
   Internal helpers (used in kineticheap.c)
───────────────────────────────────────── */
double kh_eval_key(KineticHeap *kh, int idx);
double kh_cert_time(KineticHeap *kh, int parent, int child);
void   kh_issue_cert(KineticHeap *kh, int parent, int child);
void   kh_revoke_cert(KineticHeap *kh, int parent, int child);
int    kh_sift_up(KineticHeap *kh, int idx);
void   kh_sift_down(KineticHeap *kh, int idx);
void   kh_apply_lazy(KineticHeap *kh, int idx);

/* ─────────────────────────────────────────
   Invariant assertion macros (debug mode)
───────────────────────────────────────── */
#ifdef KH_DEBUG
  #include <assert.h>
  #define KH_ASSERT_HEAP_ORDER(kh, i) \
      assert(kh_eval_key(kh, (i)/2) <= kh_eval_key(kh, i))
  #define KH_ASSERT_CERT(kh, p, c) \
      assert(kh->nodes[p].cert != NULL || kh->nodes[c].cert != NULL)
#else
  #define KH_ASSERT_HEAP_ORDER(kh, i) ((void)0)
  #define KH_ASSERT_CERT(kh, p, c)    ((void)0)
#endif

#endif /* KINETICHEAP_H */