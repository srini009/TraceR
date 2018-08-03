#include<stdio.h>
#include <stdlib.h>
#if WRITE_OTF2_TRACE
#include <scorep/SCOREP_User.h>
#endif
#include<mpi.h>

#include "microbenchmarks.h"

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
compute(double target_seconds)
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

main(int argc, char **argv) {

	int my_rank, num, i;
	int *buffer = NULL;
	MPI_Request req, req2;
	MPI_Status stat, stat2;
	int number = 1;
	double starttime, endtime;

	MPI_Init(&argc, &argv);
#if WRITE_OTF2_TRACE
  SCOREP_RECORDING_OFF();
#endif
	MPI_Comm_size(MPI_COMM_WORLD, &num);
	if(num != 2) {
		printf("Example must be run with 2 processes only.\n");
		return 0;
	}

	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
        #define DATA_SIZE 100000000
	buffer = (int*)malloc(DATA_SIZE*sizeof(int));
	for(i=0; i < DATA_SIZE; i++)
		buffer[i] = 0;

	init_arrays();

	starttime = MPI_Wtime();

#if WRITE_OTF2_TRACE
  SCOREP_RECORDING_ON();
#endif
        MPI_Barrier(MPI_COMM_WORLD);
	for(i=0; i < NUM_ITERS; i++) {

	if(my_rank == 1) {
	//Send
                compute(WAIT_TIME);
		MPI_Isend(buffer, DATA_SIZE, MPI_INT, 0, 123, MPI_COMM_WORLD, &req);
		compute(COMPUTE_TIME);
		MPI_Wait(&req, &stat);
	} else if(my_rank == 0) {
	//Recv
		MPI_Irecv(buffer, DATA_SIZE, MPI_INT, 1, 123, MPI_COMM_WORLD, &req2);
                compute(COMPUTE_TIME);
                MPI_Wait(&req2, &stat2);
	}

		MPI_Barrier(MPI_COMM_WORLD);
	}

	MPI_Barrier(MPI_COMM_WORLD);
#if WRITE_OTF2_TRACE
    SCOREP_RECORDING_OFF();
#endif
	endtime = MPI_Wtime();

	if(my_rank == 0) printf("Done in %f seconds.\n", endtime - starttime);

	free(buffer);

	MPI_Finalize(); 
}
