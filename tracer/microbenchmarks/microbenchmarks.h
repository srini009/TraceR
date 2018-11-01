#include <unistd.h>

#define NUM_ITERS 20
#define DIM 25

#define COMPUTE_TIME 0.1
#define WAIT_TIME 0.1
#define DATA_SIZE 200000000

static float **a = NULL, *x = NULL, *y = NULL;

void
init_arrays()
{
    int i = 0, j = 0;

    if (a == NULL) {
        a = malloc(DIM * sizeof(float *));
        for (i = 0; i < DIM; i++) {
            a[i] = malloc(DIM * sizeof(float));
        }
    }

    if (x == NULL) {
        x = malloc(DIM * sizeof(float));
    }
    if (y == NULL) {
        y = malloc(DIM * sizeof(float));
    }

    for (i = 0; i < DIM; i++) {
        x[i] = y[i] = 1.0f;
        for (j = 0; j < DIM; j++) {
            a[i][j] = 2.0f;
        }
    }
}

void
compute_on_host()
{
    int i = 0, j = 0;
    for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++)
            x[i] = x[i] + a[i][j]*a[j][i] + y[j];
}

static inline void
compute_(double target_seconds)
{
    double t1 = 0.0, t2 = 0.0;
    double time_elapsed = 0.0;
    while (time_elapsed < target_seconds) {
        t1 = MPI_Wtime();
        compute_on_host();
        t2 = MPI_Wtime();
        time_elapsed += (t2-t1);
    }
}

static inline void
compute(double target_seconds)
{
  long int t = target_seconds*1000000;
  usleep(t);
}
