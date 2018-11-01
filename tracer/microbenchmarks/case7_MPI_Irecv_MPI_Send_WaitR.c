#include<stdio.h>
#include <stdlib.h>
#if WRITE_OTF2_TRACE
#include <scorep/SCOREP_User.h>
#endif
#include<mpi.h>

#include "microbenchmarks.h"

main(int argc, char **argv) {

	int my_rank, num, i;
	int *buffer = NULL;
	MPI_Request req1, req2;
	MPI_Status stat1, stat2;
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
	for(i=0; i < DATA_SIZE; i++)
		buffer[i] = 0;

	init_arrays();

	starttime = MPI_Wtime();

#if WRITE_OTF2_TRACE
  SCOREP_RECORDING_ON();
#endif
        MPI_Barrier(MPI_COMM_WORLD);
	for(i=0; i < NUM_ITERS; i++) {

           compute(WAIT_TIME);
           if(my_rank == 0) {
	     MPI_Irecv(buffer, DATA_SIZE, MPI_INT, 1, 123, MPI_COMM_WORLD, &req1);
             compute(COMPUTE_TIME);
	     MPI_Send(buffer, DATA_SIZE, MPI_INT, 1, 321, MPI_COMM_WORLD);
             MPI_Wait(&req1, &stat1);
           } else {
	     MPI_Irecv(buffer, DATA_SIZE, MPI_INT, 0, 321, MPI_COMM_WORLD, &req2);
             compute(COMPUTE_TIME);
	     MPI_Send(buffer, DATA_SIZE, MPI_INT, 0, 123, MPI_COMM_WORLD);
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
