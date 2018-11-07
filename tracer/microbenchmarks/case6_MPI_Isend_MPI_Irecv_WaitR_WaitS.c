#include<stdio.h>
#include<stdlib.h>
#include<mpi.h>
#include "microbenchmarks.h"

#if WRITE_OTF2_TRACE
#include <scorep/SCOREP_User.h>
#endif

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
	buffer = (int*)malloc(DATA_SIZE*sizeof(int));

	init_arrays();

	starttime = MPI_Wtime();
#if WRITE_OTF2_TRACE
  SCOREP_RECORDING_ON();
#endif

	MPI_Barrier(MPI_COMM_WORLD);
	for(i=0; i < NUM_ITERS; i++) {

	if(my_rank == 1) {
	//Send
		MPI_Isend(buffer, DATA_SIZE, MPI_INT, 0, 123, MPI_COMM_WORLD, &req);
		compute(3*COMPUTE_TIME);
		MPI_Wait(&req, &stat);
	} else if(my_rank == 0) {
	//Recv
                compute(WAIT_TIME);
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
