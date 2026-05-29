#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define TMAX   900.0
#define TRIALS 30

static double now_s(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC,&ts);
    return ts.tv_sec+ts.tv_nsec*1e-9;
}

/* reuse exact same KH and FH code from bench_network_trace.c */
/* ── Fibonacci Heap ── */
typedef struct FN{
    double key; long payload;
    int deg,mark;
    struct FN*par,*chd,*L,*R;
} FN;
typedef struct{ FN*min; int sz; } FH;
static FN* fn_new(double k,long p){FN*n=calloc(1,sizeof(FN));n->key=k;n->payload=p;n->L=n->R=n;return n;}
static void fh_add_root(FH*h,FN*n){if(!h->min){h->min=n;n->L=n->R=n;return;}n->L=h->min;n->R=h->min->R;h->min->R->L=n;h->min->R=n;if(n->key<h->min->key)h->min=n;}
static FN* fh_ins(FH*h,double k,long p){FN*n=fn_new(k,p);fh_add_root(h,n);h->sz++;return n;}
static void fh_link(FN*y,FN*x){y->L->R=y->R;y->R->L=y->L;y->par=x;if(!x->chd){x->chd=y;y->L=y->R=y;}else{y->L=x->chd;y->R=x->chd->R;x->chd->R->L=y;x->chd->R=y;}x->deg++;y->mark=0;}
static void fh_consolidate(FH*h){int MD=64;FN**A=calloc(MD,sizeof(FN*));int cnt=0;FN*it=h->min;do{cnt++;it=it->R;}while(it!=h->min);FN**R=malloc(cnt*sizeof(FN*));it=h->min;for(int i=0;i<cnt;i++){R[i]=it;it=it->R;}for(int i=0;i<cnt;i++){FN*x=R[i];x->L=x->R=x;x->par=NULL;int d=x->deg;while(d<MD&&A[d]){FN*y=A[d];if(x->key>y->key){FN*t=x;x=y;y=t;}fh_link(y,x);A[d]=NULL;d++;}if(d<MD)A[d]=x;}free(R);h->min=NULL;for(int i=0;i<MD;i++){if(!A[i])continue;A[i]->L=A[i]->R=A[i];A[i]->par=NULL;fh_add_root(h,A[i]);}free(A);}
static double fh_extract(FH*h,long*pout){if(!h->min){*pout=-1;return 1e18;}FN*z=h->min;if(z->chd){FN*c=z->chd,*first=c;do{FN*nx=c->R;c->par=NULL;c->L=h->min;c->R=h->min->R;h->min->R->L=c;h->min->R=c;c=nx;}while(c!=first);}z->L->R=z->R;z->R->L=z->L;if(z==z->R)h->min=NULL;else{h->min=z->R;fh_consolidate(h);}h->sz--;*pout=z->payload;double k=z->key;free(z);return k;}

#define DEFER_WINDOW 30.0
typedef struct{double a,b,tlast;int id;}KN;
typedef struct{KN*nd;int*hp,*pos,n,cap,lpe;FH eq;long long csched,cstale,cdefer,clazy,csup,cevent;}KH;
static KH* kh_mk(int cap,int lpe){KH*k=calloc(1,sizeof(KH));k->cap=cap;k->lpe=lpe;k->nd=calloc(cap+2,sizeof(KN));k->hp=calloc(cap+2,sizeof(int));k->pos=calloc(cap+2,sizeof(int));return k;}
static void kh_free(KH*k){while(k->eq.min){long p;fh_extract(&k->eq,&p);}free(k->nd);free(k->hp);free(k->pos);free(k);}
static double key_at(KH*k,int id,double t){return k->nd[id].a+k->nd[id].b*t;}
static void cert_ins(KH*k,int i,int j,double t){double bi=k->nd[i].b,bj=k->nd[j].b;if(fabs(bi-bj)<1e-12){if(k->lpe)k->csup++;return;}double ai=key_at(k,i,t),aj=key_at(k,j,t);double dt=(aj-ai)/(bi-bj);double tc=t+dt;if(tc<=t+1e-9){if(k->lpe)k->csup++;return;}if(k->lpe&&(tc-t)>DEFER_WINDOW){k->csup++;k->cdefer++;return;}long payload=(long)i*1000000+(long)j;fh_ins(&k->eq,tc,payload);k->csched++;}
static void sift_dn(KH*k,int p,double t){int n=k->n;for(;;){int s=p,l=2*p,r=2*p+1;if(l<=n&&key_at(k,k->hp[l],t)<key_at(k,k->hp[s],t))s=l;if(r<=n&&key_at(k,k->hp[r],t)<key_at(k,k->hp[s],t))s=r;if(s==p)break;int tmp=k->hp[p];k->hp[p]=k->hp[s];k->pos[k->hp[s]]=p;k->hp[s]=tmp;k->pos[tmp]=s;p=s;}}
static void sift_up(KH*k,int p,double t){while(p>1){int par=p/2;if(key_at(k,k->hp[p],t)<key_at(k,k->hp[par],t)){int tmp=k->hp[p];k->hp[p]=k->hp[par];k->pos[k->hp[par]]=p;k->hp[par]=tmp;k->pos[tmp]=par;p=par;}else break;}}
static void kh_build(KH*k,double*as,double*bs,int n,double t0){k->n=n;for(int i=1;i<=n;i++){k->nd[i].a=as[i-1];k->nd[i].b=bs[i-1];k->nd[i].tlast=t0;k->nd[i].id=i;k->hp[i]=i;k->pos[i]=i;}for(int i=n/2;i>=1;i--)sift_dn(k,i,t0);for(int i=2;i<=n;i++)cert_ins(k,k->hp[i/2],k->hp[i],t0);}
static double kh_step(KH*k,double tmax){if(!k->eq.min)return -1.0;long payload;double te=fh_extract(&k->eq,&payload);if(te>tmax)return -1.0;int i=(int)(payload/1000000),j=(int)(payload%1000000);int pi=k->pos[i],pj=k->pos[j];int adj=(pi>1&&k->hp[pi/2]==j)||(pj>1&&k->hp[pj/2]==i);if(!adj){k->cstale++;return te;}k->cevent++;if(k->lpe){k->clazy+=2;}k->hp[pi]=j;k->pos[j]=pi;k->hp[pj]=i;k->pos[i]=pj;int pp=(pi<pj)?pi:pj;if(pp>1)cert_ins(k,k->hp[pp/2],k->hp[pp],te);int lc=2*pp,rc=2*pp+1;if(lc<=k->n)cert_ins(k,k->hp[pp],k->hp[lc],te);if(rc<=k->n)cert_ins(k,k->hp[pp],k->hp[rc],te);return te;}

/* ── Load MAWI trajectories from CSV ── */
typedef struct{double a,b;}Traj;

static Traj* load_mawi(const char*path, int*out_n){
    FILE*f=fopen(path,"r");
    if(!f){fprintf(stderr,"Cannot open %s\n",path);exit(1);}
    char line[256];
    fgets(line,sizeof(line),f); /* skip header */
    Traj*T=malloc(20000*sizeof(Traj));
    int n=0;
    while(fgets(line,sizeof(line),f)){
        double a,b;
        if(sscanf(line,"%lf,%lf",&a,&b)==2)
            T[n].a=a, T[n].b=b, n++;
    }
    fclose(f);
    *out_n=n;
    return T;
}

typedef struct{double mean,lo,hi;long long cs,ce,csup,cst;double srate;}Res;

static Res bench(Traj*T,int n,int lpe){
    double times[TRIALS];
    long long cs=0,ce=0,csup=0,cst=0;
    for(int tr=0;tr<TRIALS;tr++){
        double*as=malloc(n*sizeof(double));
        double*bs=malloc(n*sizeof(double));
        for(int i=0;i<n;i++){as[i]=T[i].a;bs[i]=T[i].b;}
        KH*k=kh_mk(n+2,lpe);
        kh_build(k,as,bs,n,0.0);
        double t0=now_s(),t=0.0;
        for(;;){double tn=kh_step(k,TMAX);if(tn<0)break;t=tn;}
        times[tr]=now_s()-t0;
        if(tr==TRIALS-1){cs=k->csched;ce=k->cevent;csup=k->csup;cst=k->cstale;}
        kh_free(k);free(as);free(bs);
    }
    double s=0;for(int i=0;i<TRIALS;i++)s+=times[i];
    double mn=s/TRIALS,v=0;
    for(int i=0;i<TRIALS;i++)v+=(times[i]-mn)*(times[i]-mn);
    v/=(TRIALS-1);double se=sqrt(v/TRIALS);
    Res r;r.mean=mn;r.lo=mn-1.96*se;r.hi=mn+1.96*se;
    r.cs=cs;r.ce=ce;r.csup=csup;r.cst=cst;
    r.srate=(cs+csup>0)?100.0*csup/(cs+csup):0.0;
    return r;
}

int main(void){
    int n;
    Traj*T=load_mawi("/mnt/d/mawi_trajectories.csv",&n);
    printf("Loaded %d MAWI trajectories\n\n",n);

    Res kh=bench(T,n,1);
    Res b2=bench(T,n,0);
    double spd=b2.mean/kh.mean;
    double red=(b2.cs>0&&kh.cs>0)?(double)b2.cs/kh.cs:0.0;

    printf("  Metric                 KH++          B2\n");
    printf("  ─────────────────────────────────────────\n");
    printf("  Csched              %8lld      %8lld\n",kh.cs,b2.cs);
    printf("  Cevent              %8lld      %8lld\n",kh.ce,b2.ce);
    printf("  Csuppressed(LPE)    %8lld           0\n",kh.csup);
    printf("  Cstale              %8lld      %8lld\n",kh.cst,b2.cst);
    printf("  Supp. rate          %7.1f%%          0%%\n",kh.srate);
    printf("  Mean time (s)       %8.4f      %8.4f\n",kh.mean,b2.mean);
    printf("  95%% CI (KH++)   [%.4f, %.4f]\n",kh.lo,kh.hi);
    printf("  Speedup             %8.2fx\n",spd);
    printf("  Csched reduction    %8.2fx fewer for KH++\n",red);

    free(T);
    return 0;
}
