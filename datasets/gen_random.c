/* gen_random.c
 * Synthetic dataset generator for KineticHeap++ benchmarks.
 * Generates random linear trajectories ki(t) = ai + bi*t
 * and saves to CSV format.
 *
 * Usage: ./build/gen_random <n> <output_file>
 * Example: ./build/gen_random 10000 datasets/random_10k.csv
 * gen_random.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <n> <output_file>\n", argv[0]);
        return 1;
    }

    int   n        = atoi(argv[1]);
    char *filename = argv[2];


    FILE *fp = fopen(filename, "w");
    if (!fp) { fprintf(stderr, "Cannot open %s\n", filename); return 1; }

    srand(42);  /* fixed seed for reproducibility */

    fprintf(fp, "id,intercept,slope\n");
    for (int i = 0; i < n; i++) {
        double a = (double)rand() / RAND_MAX * 1000.0;
        double b = ((double)rand() / RAND_MAX * 2.0) - 1.0;
        fprintf(fp, "%d,%.6f,%.6f\n", i, a, b);
    }

    fclose(fp);
    printf("Generated %d trajectories -> %s\n", n, filename);
    return 0;
}