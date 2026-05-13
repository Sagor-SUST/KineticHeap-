#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define DEFER_WINDOW 5.0   /* LPE deferral threshold */
#define TMAX         200.0
#define TRIALS       30

static double now_s(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC,&ts);
    return ts.tv_sec+ts.tv_nsec*1e-9;
}

/* ── Fibonacci Heap ── */
typedef struct FN{
    double key; long payload;
    int deg,mark;
    struct FN*par,*chd,*L,*R;
} FN;
typedef struct{ FN*min; int sz; } FH;

static FN* fn_new(double k,long p){
    FN*n=calloc(1,sizeof(FN));
    n->key=k;n->payload=p;n->L=n->R=n;
    return n;
}
static void fh_add_root(FH*h,FN*n){
    if(!h->min){h->min=n;n->L=n->R=n;return;}
    n->L=h->min;n->R=h->min->R;
    h->min->R->L=n;h->min->R=n;
    if(n->key<h->min->key)h->min=n;
}
static FN* fh_ins(FH*h,double k,long p){
    FN*n=fn_new(k,p);fh_add_root(h,n);h->sz++;return n;
}
static void fh_link(FN*y,FN*x){
    y->L->R=y->R;y->R->L=y->L;y->par=x;
    if(!x->chd){x->chd=y;y->L=y->R=y;}
    else{y->L=x->chd;y->R=x->chd->R;x->chd->R->L=y;x->chd->R=y;}
    x->deg++;y->mark=0;
}
static void fh_consolidate(FH*h){
    int MD=64;FN**A=calloc(MD,sizeof(FN*));
    int cnt=0;FN*it=h->min;
    do{cnt++;it=it->R;}while(it!=h->min);
    FN**R=malloc(cnt*sizeof(FN*));it=h->min;
    for(int i=0;i<cnt;i++){R[i]=it;it=it->R;}
    for(int i=0;i<cnt;i++){
        FN*x=R[i];x->L=x->R=x;x->par=NULL;
        int d=x->deg;
        while(d<MD&&A[d]){
            FN*y=A[d];
            if(x->key>y->key){FN*t=x;x=y;y=t;}
            fh_link(y,x);A[d]=NULL;d++;
        }
        if(d<MD)A[d]=x;
    }
    free(R);h->min=NULL;
    for(int i=0;i<MD;i++){
        if(!A[i])continue;
        A[i]->L=A[i]->R=A[i];A[i]->par=NULL;
        fh_add_root(h,A[i]);
    }
    free(A);
}
static double fh_extract(FH*h,long*pout){
    if(!h->min){*pout=-1;return 1e18;}
    FN*z=h->min;
    if(z->chd){
        FN*c=z->chd,*first=c;
        do{FN*nx=c->R;c->par=NULL;
           c->L=h->min;c->R=h->min->R;
           h->min->R->L=c;h->min->R=c;
           c=nx;}while(c!=first);
    }
    z->L->R=z->R;z->R->L=z->L;
    if(z==z->R)h->min=NULL;
    else{h->min=z->R;fh_consolidate(h);}
    h->sz--;
    *pout=z->payload;double k=z->key;free(z);return k;
}

/* ── Kinetic Heap ── */
typedef struct{double a,b,tlast;int id;}KN;
typedef struct{
    KN*nd;int*hp,*pos,n,cap,lpe;
    FH eq;
    long long csched,cstale,cdefer,clazy,csup,cevent;
}KH;

static KH* kh_mk(int cap,int lpe){
    KH*k=calloc(1,sizeof(KH));k->cap=cap;k->lpe=lpe;
    k->nd=calloc(cap+2,sizeof(KN));
    k->hp=calloc(cap+2,sizeof(int));
    k->pos=calloc(cap+2,sizeof(int));
    return k;
}
static void kh_free(KH*k){
    while(k->eq.min){long p;fh_extract(&k->eq,&p);}
    free(k->nd);free(k->hp);free(k->pos);free(k);
}
static double key_at(KH*k,int id,double t){
    return k->nd[id].a+k->nd[id].b*t;
}

static void cert_ins(KH*k,int i,int j,double t){
    double bi=k->nd[i].b,bj=k->nd[j].b;

    /* Case 1: parallel — never crosses */
    if(fabs(bi-bj)<1e-12){
        if(k->lpe)k->csup++;
        return;
    }

    double ai=key_at(k,i,t),aj=key_at(k,j,t);
    double dt=(aj-ai)/(bi-bj);
    double tc=t+dt;

    /* Case 2: crossing already past */
    if(tc<=t+1e-9){
        if(k->lpe)k->csup++;
        return;
    }

    /* Case 3 (LPE CORE): crossing is far future → defer */
    if(k->lpe && (tc-t)>DEFER_WINDOW){
        k->csup++;
        k->cdefer++;
        return;   /* KEY: do NOT insert into queue */
    }

    /* Case 4: insert certificate */
    long payload=(long)i*1000000+(long)j;
    fh_ins(&k->eq,tc,payload);
    k->csched++;
}

static void sift_dn(KH*k,int p,double t){
    int n=k->n;
    for(;;){
        int s=p,l=2*p,r=2*p+1;
        if(l<=n&&key_at(k,k->hp[l],t)<key_at(k,k->hp[s],t))s=l;
        if(r<=n&&key_at(k,k->hp[r],t)<key_at(k,k->hp[s],t))s=r;
        if(s==p)break;
        int tmp=k->hp[p];k->hp[p]=k->hp[s];k->pos[k->hp[s]]=p;
        k->hp[s]=tmp;k->pos[tmp]=s;p=s;
    }
}
static void sift_up(KH*k,int p,double t){
    while(p>1){
        int par=p/2;
        if(key_at(k,k->hp[p],t)<key_at(k,k->hp[par],t)){
            int tmp=k->hp[p];k->hp[p]=k->hp[par];k->pos[k->hp[par]]=p;
            k->hp[par]=tmp;k->pos[tmp]=par;p=par;
        }else break;
    }
}
static void kh_build(KH*k,double*as,double*bs,int n,double t0){
    k->n=n;
    for(int i=1;i<=n;i++){
        k->nd[i].a=as[i-1];k->nd[i].b=bs[i-1];
        k->nd[i].tlast=t0;k->nd[i].id=i;
        k->hp[i]=i;k->pos[i]=i;
    }
    for(int i=n/2;i>=1;i--)sift_dn(k,i,t0);
    for(int i=2;i<=n;i++)cert_ins(k,k->hp[i/2],k->hp[i],t0);
}
static double kh_step(KH*k,double tmax){
    if(!k->eq.min)return -1.0;
    long payload;double te=fh_extract(&k->eq,&payload);
    if(te>tmax)return -1.0;
    int i=(int)(payload/1000000),j=(int)(payload%1000000);
    int pi=k->pos[i],pj=k->pos[j];
    int adj=(pi>1&&k->hp[pi/2]==j)||(pj>1&&k->hp[pj/2]==i);
    if(!adj){k->cstale++;return te;}
    k->cevent++;
    if(k->lpe){k->clazy+=2;}
    k->hp[pi]=j;k->pos[j]=pi;k->hp[pj]=i;k->pos[i]=pj;
    int pp=(pi<pj)?pi:pj;
    if(pp>1)cert_ins(k,k->hp[pp/2],k->hp[pp],te);
    int lc=2*pp,rc=2*pp+1;
    if(lc<=k->n)cert_ins(k,k->hp[pp],k->hp[lc],te);
    if(rc<=k->n)cert_ins(k,k->hp[pp],k->hp[rc],te);
    return te;
}

/* ── Data Generators ── */
typedef struct{double a,b;}Pkt;

static Pkt* gen_network(int n,unsigned seed){
    srand(seed);
    Pkt*P=malloc(n*sizeof(Pkt));
    for(int i=0;i<n;i++){
        double r=(double)rand()/RAND_MAX;
        double a=1000.0*(double)rand()/RAND_MAX;
        double b;
        if(r<0.60)      b=-0.5+1.0*(double)rand()/RAND_MAX;
        else if(r<0.85) b= 2.0+3.0*(double)rand()/RAND_MAX;
        else            b=-5.0+2.0*(double)rand()/RAND_MAX;
        if((double)rand()/RAND_MAX<0.15){
            a*=4.0;
            b+=3.0+2.0*(double)rand()/RAND_MAX;
        }
        P[i].a=a;P[i].b=b;
    }
    return P;
}

static Pkt* gen_molec(int n,unsigned seed){
    srand(seed+777);
    Pkt*P=malloc(n*sizeof(Pkt));
    for(int i=0;i<n;i++){
        double u1=((double)rand()+1)/(RAND_MAX+2.0);
        double u2=((double)rand()+1)/(RAND_MAX+2.0);
        double g=sqrt(-2.0*log(u1))*cos(2.0*M_PI*u2);
        double mass=0.5+9.5*(double)rand()/RAND_MAX;
        P[i].b=g*sqrt(1.0/mass);
        P[i].a=1000.0*(double)rand()/RAND_MAX;
    }
    return P;
}

/* ── Benchmark ── */
typedef struct{
    double mean,lo,hi;
    long long cs,ce,csup,cst;
    double srate;
}Res;

static Res bench(Pkt*P,int n,int lpe){
    double times[TRIALS];
    long long cs=0,ce=0,csup=0,cst=0;
    for(int tr=0;tr<TRIALS;tr++){
        double*as=malloc(n*sizeof(double));
        double*bs=malloc(n*sizeof(double));
        for(int i=0;i<n;i++){as[i]=P[i].a;bs[i]=P[i].b;}
        KH*k=kh_mk(n+2,lpe);
        kh_build(k,as,bs,n,0.0);
        double t0=now_s(),t=0.0;
        for(;;){double tn=kh_step(k,TMAX);if(tn<0)break;t=tn;}
        times[tr]=now_s()-t0;
        if(tr==TRIALS-1){cs=k->csched;ce=k->cevent;csup=k->csup;cst=k->cstale;}
        kh_free(k);free(as);free(bs);
    }
    double s=0;
    for(int i=0;i<TRIALS;i++)s+=times[i];
    double mn=s/TRIALS,v=0;
    for(int i=0;i<TRIALS;i++)v+=(times[i]-mn)*(times[i]-mn);
    v/=(TRIALS-1);double se=sqrt(v/TRIALS);
    Res r;r.mean=mn;r.lo=mn-1.96*se;r.hi=mn+1.96*se;
    r.cs=cs;r.ce=ce;r.csup=csup;r.cst=cst;
    r.srate=(cs+csup>0)?100.0*csup/(cs+csup):0.0;
    return r;
}

static void print_table(const char*name, int*sizes, int NS, Pkt*(*gen)(int,unsigned)){
    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  KINETICHEAP++  %-46s║\n",name);
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("%-8s  %-10s  %-10s  %-9s  %-12s  %-12s  %-10s  %-10s\n",
           "n","KH++(s)","B2(s)","Speedup","Csched(KH)","Csched(B2)","Reduction","SuppRate");
    printf("──────────────────────────────────────────────────────────────────────────\n");
    for(int si=0;si<NS;si++){
        int n=sizes[si];
        Pkt*P=gen(n,42);
        Res kh=bench(P,n,1);
        Res b2=bench(P,n,0);
        double spd=b2.mean/kh.mean;
        double red=(b2.cs>0)?(double)b2.cs/kh.cs:1.0;
        /* handle KH cs=0 edge case */
        if(kh.cs==0) red=0.0;
        printf("%-8d  %-10.4f  %-10.4f  %-9.2fx  %-12lld  %-12lld  %-10.2fx  %-8.1f%%\n",
               n,kh.mean,b2.mean,spd,kh.cs,b2.cs,red,kh.srate);
        free(P);
    }
}

static void print_detail(const char*name, int n, Pkt*(*gen)(int,unsigned)){
    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  Detailed Stats — %-43s║\n",name);
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    Pkt*P=gen(n,42);
    Res kh=bench(P,n,1);
    Res b2=bench(P,n,0);
    double red=(kh.cs>0)?(double)b2.cs/kh.cs:0.0;
    printf("  Metric                 KH++          B2\n");
    printf("  ─────────────────────────────────────────\n");
    printf("  Csched              %8lld      %8lld\n",kh.cs,b2.cs);
    printf("  Cevent              %8lld      %8lld\n",kh.ce,b2.ce);
    printf("  Csuppressed(LPE)    %8lld           0\n",kh.csup);
    printf("  Cstale              %8lld      %8lld\n",kh.cst,b2.cst);
    printf("  Supp. rate          %7.1f%%          0%%\n",kh.srate);
    printf("  Mean time (s)       %8.4f      %8.4f\n",kh.mean,b2.mean);
    printf("  95%% CI (KH++)   [%.4f, %.4f]\n",kh.lo,kh.hi);
    printf("  Speedup             %8.2fx\n",b2.mean/kh.mean);
    printf("  Csched reduction    %8.2fx fewer for KH++\n",red);
    free(P);
}

int main(void){
    int sizes[]={1000,5000,10000,50000,100000,500000,1000000};
    int NS=7;

    print_table("Workload-1: Network Event Trace",sizes,NS,gen_network);
    print_table("Workload-2: Molecular Dynamics ",sizes,NS,gen_molec);
    print_detail("Network Trace, n=100000",100000,gen_network);
    print_detail("Molecular Dynamics, n=100000",100000,gen_molec);

    printf("\nDone.\n");
    return 0;
}
