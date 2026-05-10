/*
 * B2 Baseline: Fibonacci Heap Priority Queue
 * KineticHeap++ paper — baseline comparison
 * Windows + Linux compatible timing
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* ── Cross-platform timing ────────────────────────────────────────────────── */
#ifdef _WIN32
#include <windows.h>
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

/* ── Node ─────────────────────────────────────────────────────────────────── */
typedef struct FHNode {
    int    id;
    double key;
    int    degree;
    int    marked;
    struct FHNode *parent;
    struct FHNode *child;
    struct FHNode *left;
    struct FHNode *right;
} FHNode;

typedef struct {
    FHNode *min;
    int     size;
} FibHeap;

static FHNode *fh_new_node(int id, double key) {
    FHNode *n  = (FHNode *)malloc(sizeof(FHNode));
    n->id=id; n->key=key; n->degree=0; n->marked=0;
    n->parent=NULL; n->child=NULL; n->left=n; n->right=n;
    return n;
}

static void fh_link(FHNode *y, FHNode *x) {
    y->left->right=y->right; y->right->left=y->left;
    y->parent=x;
    if (!x->child) { x->child=y; y->left=y; y->right=y; }
    else {
        y->left=x->child; y->right=x->child->right;
        x->child->right->left=y; x->child->right=y;
    }
    x->degree++; y->marked=0;
}

static FibHeap *fh_create(void) {
    FibHeap *h=(FibHeap*)malloc(sizeof(FibHeap)); h->min=NULL; h->size=0; return h;
}

static void fh_insert(FibHeap *h, FHNode *n) {
    if (!h->min) { h->min=n; n->left=n; n->right=n; }
    else {
        n->right=h->min; n->left=h->min->left;
        h->min->left->right=n; h->min->left=n;
        if (n->key < h->min->key) h->min=n;
    }
    h->size++;
}

static void fh_consolidate(FibHeap *h) {
    int max_deg=(int)(log2((double)h->size)+2)+10;
    FHNode **A=(FHNode**)calloc(max_deg,sizeof(FHNode*));
    int rc=0; FHNode *cur=h->min;
    do { rc++; cur=cur->right; } while(cur!=h->min);
    FHNode **roots=(FHNode**)malloc(rc*sizeof(FHNode*));
    cur=h->min;
    for(int i=0;i<rc;i++){roots[i]=cur;cur=cur->right;}
    for(int i=0;i<rc;i++){
        FHNode *x=roots[i]; int d=x->degree;
        while(d<max_deg&&A[d]){
            FHNode *y=A[d];
            if(x->key>y->key){FHNode*tmp=x;x=y;y=tmp;}
            fh_link(y,x); A[d]=NULL; d++;
        }
        if(d<max_deg) A[d]=x;
    }
    h->min=NULL;
    for(int i=0;i<max_deg;i++){
        if(!A[i]) continue;
        A[i]->left=A[i]; A[i]->right=A[i];
        if(!h->min){ h->min=A[i]; }
        else {
            A[i]->right=h->min; A[i]->left=h->min->left;
            h->min->left->right=A[i]; h->min->left=A[i];
            if(A[i]->key<h->min->key) h->min=A[i];
        }
    }
    free(A); free(roots);
}

static FHNode *fh_extract_min(FibHeap *h) {
    FHNode *z=h->min; if(!z) return NULL;
    if(z->child){
        FHNode *child=z->child; int deg=z->degree;
        for(int i=0;i<deg;i++){
            FHNode *next=child->right;
            child->left=h->min; child->right=h->min->right;
            h->min->right->left=child; h->min->right=child;
            child->parent=NULL; child=next;
        }
    }
    z->left->right=z->right; z->right->left=z->left;
    if(z==z->right){ h->min=NULL; }
    else { h->min=z->right; fh_consolidate(h); }
    h->size--; return z;
}

static void fh_free_nodes(FHNode *n) {
    if(!n) return;
    FHNode *start=n, *cur=n;
    do { FHNode *next=cur->right; fh_free_nodes(cur->child); free(cur); cur=next; } while(cur!=start);
}

static void fh_destroy(FibHeap *h) { if(h->min) fh_free_nodes(h->min); free(h); }

int main(void) {
    int sizes[]={10000,30000,100000,300000,1000000,3000000,10000000};
    int num_sizes=7;
    printf("B2 Fibonacci Heap Baseline\n");
    printf("%-12s %-12s %-12s\n","n","time(s)","ops");
    for(int s=0;s<num_sizes;s++){
        int n=sizes[s]; srand(42);
        FibHeap *h=fh_create();
        FHNode **nodes=(FHNode**)malloc(n*sizeof(FHNode*));
        double t0=get_time();
        for(int i=0;i<n;i++){
            double key=(double)rand()/RAND_MAX*1000.0;
            nodes[i]=fh_new_node(i,key); fh_insert(h,nodes[i]);
        }
        int ops=0;
        for(int i=0;i<n/2;i++){
            FHNode *m=fh_extract_min(h); if(m){free(m);ops++;}
        }
        double elapsed=get_time()-t0;
        printf("%-12d %-12.4f %-12d\n",n,elapsed,ops);
        fflush(stdout);
        fh_destroy(h); free(nodes);
    }
    return 0;
}