CC=mpicc
SCOREP=scorep
SCOREPFLAGS="--user --nocompiler --noopenmp --nopomp --nocuda --noopenacc --noopencl --nomemory mpicxx -O2 -DWRITE_OTF2_TRACE=1 -I/include -I/include/scorep -DSCOREP_USER_ENABLE  -fpermissive"

all: case1_MPI_Irecv_WaitR_MPI_Isend_WaitS.c case2_MPI_Isend_WaitS_MPI_Irecv_WaitR.c case3_MPI_Irecv_MPI_Isend_WaitR_WaitS.c \
     case4_MPI_Isend_MPI_Irecv_WaitS_WaitR.c case5_MPI_Irecv_MPI_Isend_WaitS_WaitR.c case6_MPI_Isend_MPI_Irecv_WaitR_WaitS.c 
	$(CC) case1_MPI_Irecv_WaitR_MPI_Isend_WaitS.c -o case1.out
	$(CC) case2_MPI_Isend_WaitS_MPI_Irecv_WaitR.c -o case2.out
	$(CC) case3_MPI_Irecv_MPI_Isend_WaitR_WaitS.c -o case3.out
	$(CC) case4_MPI_Isend_MPI_Irecv_WaitS_WaitR.c -o case4.out
	$(CC) case5_MPI_Irecv_MPI_Isend_WaitS_WaitR.c -o case5.out
	$(CC) case6_MPI_Isend_MPI_Irecv_WaitR_WaitS.c -o case6.out

debug: case1_MPI_Irecv_WaitR_MPI_Isend_WaitS.c case2_MPI_Isend_WaitS_MPI_Irecv_WaitR.c case3_MPI_Irecv_MPI_Isend_WaitR_WaitS.c \
       case4_MPI_Isend_MPI_Irecv_WaitS_WaitR.c case5_MPI_Irecv_MPI_Isend_WaitS_WaitR.c case6_MPI_Isend_MPI_Irecv_WaitR_WaitS.c
	$(SCOREP) $(SCOREPFLAGS) case1_MPI_Irecv_WaitR_MPI_Isend_WaitS.c -o case1_sp.out
	$(SCOREP) $(SCOREPFLAGS) case2_MPI_Isend_WaitS_MPI_Irecv_WaitR.c -o case2_sp.out
	$(SCOREP) $(SCOREPFLAGS) case3_MPI_Irecv_MPI_Isend_WaitR_WaitS.c -o case3_sp.out
	$(SCOREP) $(SCOREPFLAGS) case4_MPI_Isend_MPI_Irecv_WaitS_WaitR.c -o case4_sp.out
	$(SCOREP) $(SCOREPFLAGS) case5_MPI_Irecv_MPI_Isend_WaitS_WaitR.c -o case5_sp.out
	$(SCOREP) $(SCOREPFLAGS) case6_MPI_Isend_MPI_Irecv_WaitR_WaitS.c -o case6_sp.out

clean:
	rm -rf *.out core* tracer_stats_* 

