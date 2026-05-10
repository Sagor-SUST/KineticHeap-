#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "kineticheap.h"

static double rand_double(double lo, double hi) {
    return lo + (hi-lo)*((double)rand()/RAND_MAX);
}
static double rand_gauss(double mu, double sigma) {
    double u1=((double)rand()+1)/((double)RAND_MAX+1);
    double u2=((double)rand()+1)/((double)RAND_MAX+1);
    return mu + sigma*sqrt(-2*log(u1))*cos(2*3.14159265*u2);
}

static void run_bench(const char *name, int n, double *as, double *bs, FILE *fp) {
    KineticHeap *kh = kh_create(n+10);
    srand(42);
    for (int i=0; i<n; i++) kh_insert(kh, as[i], bs[i], i);

    clock_t t0 = clock();
    int certs=0, max_events=n*5;
    for (int e=0; e<max_events; e++) {
        if (!kh->fheap->min) break;
        if (kh->fheap->min->key > 500.0) break;
        kh_handle_event(kh);
        certs++;
    }
    double elapsed=(double)(clock()-t0)/CLOCKS_PER_SEC;
    printf("  %-12s n=%-8d %.4f sec  %d certs\n", name, n, elapsed, certs);
    fprintf(fp, "%s,%d,%.6f,%d\n", name, n, elapsed, certs);
    kh_destroy(kh);
}

int main(void) {
    int sizes[]={10000,100000,1000000};
    int ns=3;

    FILE *fp=fopen("results/adversarial.csv","w");
    if (!fp) { fprintf(stderr,"Cannot open results/adversarial.csv\n"); return 1; }
    fprintf(fp,"workload,n,time_sec,certs\n");

    printf("=== Adversarial Workload Benchmarks ===\n\n");

    for (int s=0; s<ns; s++) {
        int n=sizes[s];
        double *as=(double*)malloc(n*sizeof(double));
        double *bs=(double*)malloc(n*sizeof(double));

        printf("--- n=%d ---\n", n);

        /* 1. Random (baseline) */
        srand(42);
        for (int i=0;i<n;i++){as[i]=rand_double(0,1000);bs[i]=rand_double(-1,1);}
        run_bench("random", n, as, bs, fp);

        /* 2. Clustered crossings — slopes near +0.5 or -0.5 */
        srand(42);
        for (int i=0;i<n;i++){
            as[i]=rand_double(0,1000);
            double mu=(i%2==0)?0.5:-0.5;
            bs[i]=rand_gauss(mu,0.05);
        }
        run_bench("clustered", n, as, bs, fp);

        /* 3. Bursty updates — most slopes ~0, few very steep */
        srand(42);
        for (int i=0;i<n;i++){
            as[i]=rand_double(0,1000);
            bs[i]=(rand()%10==0)?rand_double(-10,10):rand_double(-0.01,0.01);
        }
        run_bench("bursty", n, as, bs, fp);

        /* 4. Degenerate — very small slopes */
        srand(42);
        for (int i=0;i<n;i++){
            as[i]=rand_double(0,1000);
            bs[i]=rand_double(-1,1)*1e-3;
        }
        run_bench("degenerate", n, as, bs, fp);

        free(as); free(bs);
        printf("\n");
    }
    fclose(fp);
    printf("Results saved to results/adversarial.csv\n");
    return 0;
}
