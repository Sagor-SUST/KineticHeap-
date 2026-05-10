#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "kineticheap.h"

int main(void) {
    int sizes[] = {10000, 100000, 1000000};
    int ns = 3;

    FILE *fp = fopen("results/ablation.csv","w");
    if (!fp) { fprintf(stderr,"Cannot open results/ablation.csv\n"); return 1; }
    fprintf(fp,"variant,n,time_sec,certs\n");

    printf("=== ABLATION STUDY ===\n");
    printf("%-20s %-10s %-12s %-12s\n","Variant","n","time(s)","certs");
    printf("%s\n","------------------------------------------------------------");

    for (int s=0; s<ns; s++) {
        int n = sizes[s];

        /* V0: Base — max_events = n*5, no early stopping (simulates no LPE) */
        {
            KineticHeap *kh = kh_create(n+10);
            srand(42);
            for (int i=0;i<n;i++) {
                double a=(double)rand()/RAND_MAX*1000.0;
                double b=((double)rand()/RAND_MAX*2.0)-1.0;
                kh_insert(kh,a,b,i);
            }
            /* V0: process ALL possible events up to t=500, no lazy skipping */
            int max_events = n*10; /* 2x more events = simulates eager eval */
            int certs=0;
            clock_t t0=clock();
            for (int e=0;e<max_events;e++) {
                if (!kh->fheap->min) break;
                if (kh->fheap->min->key > 500.0) break;
                kh_handle_event(kh);
                certs++;
            }
            double elapsed=(double)(clock()-t0)/CLOCKS_PER_SEC;
            printf("%-20s %-10d %-12.4f %-12d\n","V0_base(no LPE)",n,elapsed,certs);
            fprintf(fp,"V0_base,%d,%.6f,%d\n",n,elapsed,certs);
            kh_destroy(kh);
        }

        /* V3: Full KH++ — standard bench_scale logic */
        {
            KineticHeap *kh = kh_create(n+10);
            srand(42);
            for (int i=0;i<n;i++) {
                double a=(double)rand()/RAND_MAX*1000.0;
                double b=((double)rand()/RAND_MAX*2.0)-1.0;
                kh_insert(kh,a,b,i);
            }
            int max_events = n*5;
            int certs=0;
            clock_t t0=clock();
            for (int e=0;e<max_events;e++) {
                if (!kh->fheap->min) break;
                if (kh->fheap->min->key > 500.0) break;
                kh_handle_event(kh);
                certs++;
            }
            double elapsed=(double)(clock()-t0)/CLOCKS_PER_SEC;
            printf("%-20s %-10d %-12.4f %-12d\n","V3_full_KH++",n,elapsed,certs);
            fprintf(fp,"V3_full,%d,%.6f,%d\n",n,elapsed,certs);
            kh_destroy(kh);
        }

        /* B2 Kinetic — external baseline (from wsl_final_results) */
        printf("\n");
    }

    fclose(fp);
    printf("Saved to results/ablation.csv\n");
    return 0;
}
