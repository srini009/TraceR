#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#if WRITE_OTF2_TRACE
#include <scorep/SCOREP_User.h>
#endif

#define max(a,b) ((a)>(b)?(a):(b))

#ifndef FIELD_WIDTH
#   define FIELD_WIDTH 20
#endif

#ifndef FLOAT_PRECISION
#   define FLOAT_PRECISION 2
#endif

#define PEERS 6
#define DIM 25
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
dummy_compute(double target_seconds)
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

void print_preamble_nbc (int rank)
{
    fprintf(stdout, "# Overall = Init + Compute + MPI_Wait\n\n");
    fprintf(stdout, "%-*s", 10, "# Size");
    fprintf(stdout, "%*s", FIELD_WIDTH, "Overall(us)");
    fprintf(stdout, "%*s", FIELD_WIDTH, "Compute(us)");
    fprintf(stdout, "%*s", FIELD_WIDTH, "Init(us)");
    fprintf(stdout, "%*s", FIELD_WIDTH, "MPI_Wait(us)");
    fprintf(stdout, "%*s", FIELD_WIDTH, "Pure Comm.(us)");
    fprintf(stdout, "%*s\n", FIELD_WIDTH, "Overlap(%)");

    fflush(stdout);
}

void
print_stats_nbc (int rank, int size, double overall_time,
                      double cpu_time, double comm_time,
                      double wait_time, double init_time)
{
    if (rank) return;

    double overlap = 0.0, test_time = 0.0;

    overlap = max(0, 100 - (((overall_time - (cpu_time - test_time)) / comm_time) * 100));

    fprintf(stdout, "%-*d", 10, size);
    fprintf(stdout, "%*.*f", FIELD_WIDTH, FLOAT_PRECISION, overall_time);

    fprintf(stdout, "%*.*f%*.*f%*.*f%*.*f%*.*f\n",
                    FIELD_WIDTH, FLOAT_PRECISION, (cpu_time - test_time),
                    FIELD_WIDTH, FLOAT_PRECISION, init_time,
                    FIELD_WIDTH, FLOAT_PRECISION, wait_time,
                    FIELD_WIDTH, FLOAT_PRECISION, comm_time,
                    FIELD_WIDTH, FLOAT_PRECISION, overlap);

    fflush(stdout);
}

void
calculate_and_print_stats(MPI_Comm comm3d, int rank, int size, int numprocs,
                          int iter_count,
                          double timer, double latency, double cpu_time,
                          double wait_time, double init_time)
{
        double tcomp_total  = (cpu_time * 1e6) / iter_count;
        double overall_time = (timer * 1e6) / iter_count;
        double wait_total   = (wait_time * 1e6) / iter_count;
        double init_total   = (init_time * 1e6) / iter_count;
        double comm_time   = latency;

        if(rank != 0) {
            MPI_Reduce(&comm_time, &comm_time, 1, MPI_DOUBLE, MPI_SUM, 0,
                comm3d);
            MPI_Reduce(&overall_time, &overall_time, 1, MPI_DOUBLE, MPI_SUM, 0,
                comm3d);
            MPI_Reduce(&tcomp_total, &tcomp_total, 1, MPI_DOUBLE, MPI_SUM, 0,
                comm3d);
            MPI_Reduce(&wait_total, &wait_total, 1, MPI_DOUBLE, MPI_SUM, 0,
                comm3d);
            MPI_Reduce(&init_total, &init_total, 1, MPI_DOUBLE, MPI_SUM, 0,
                comm3d);
        }
        else {
            MPI_Reduce(MPI_IN_PLACE, &comm_time, 1, MPI_DOUBLE, MPI_SUM, 0,
                comm3d);
            MPI_Reduce(MPI_IN_PLACE, &overall_time, 1, MPI_DOUBLE, MPI_SUM, 0,
                comm3d);
            MPI_Reduce(MPI_IN_PLACE, &tcomp_total, 1, MPI_DOUBLE, MPI_SUM, 0,
                comm3d);
            MPI_Reduce(MPI_IN_PLACE, &wait_total, 1, MPI_DOUBLE, MPI_SUM, 0,
                comm3d);
            MPI_Reduce(MPI_IN_PLACE, &init_total, 1, MPI_DOUBLE, MPI_SUM, 0,
                comm3d);
        }
        MPI_Barrier(comm3d);

        /* Overall Time (Overlapped) */
        overall_time = overall_time/numprocs;
        /* Computation Time */
        tcomp_total = tcomp_total/numprocs;
        /* Pure Communication Time */
        comm_time = comm_time/numprocs;
        /* Time for MPI_Wait() call */
        wait_total = wait_total/numprocs;
        /* Time for the NBC call */
        init_total = init_total/numprocs;

        print_stats_nbc(rank, size, overall_time, tcomp_total, comm_time,
                                    wait_total, init_total);
}

int sendrecv(MPI_Comm comm3d, int msg_size, int msg_count, int iter_count,
            int iter_warmup, int rank, int comm_size, int xrankL, int xrankR, int yrankL,
            int yrankR, int zrankL, int zrankR)
{
    int i, j;
    char *sbuf = NULL, *rbuf = NULL;
    int rdispl, sdispl, rreq_idx, sreq_idx;
    MPI_Request *sreq = NULL, *rreq = NULL;
    double latency = 0.0, latency_in_secs = 0.0;
    double comm_time = 0.0, comm_total = 0.0;
    double init_time = 0.0, init_total = 0.0;
    double comp_time = 0.0, comp_total = 0.0;
    double wait_time = 0.0, wait_total = 0.0;
    double total_time = 0.0, total_total = 0.0;

    /*allocate communication buffers*/ 
    posix_memalign((void **)&sbuf, 64, PEERS*msg_size*msg_count); 
    posix_memalign((void **)&rbuf, 64, PEERS*msg_size*msg_count); 

    sreq = (MPI_Request *) malloc(PEERS*msg_count*sizeof(MPI_Request));
    rreq = (MPI_Request *) malloc(PEERS*msg_count*sizeof(MPI_Request));

    if (sbuf == NULL || rbuf == NULL || sreq == NULL || rreq == NULL) { 
        fprintf(stderr, "[%d] Buffer and request allocation failed \n", rank); 
        exit(-1);
    }

    /* Start measuring pure communication time */
    MPI_Barrier(comm3d);

    /*send and receive messages*/  
    for (i = 0; i < (iter_count + iter_warmup); i++) {

        comm_time = MPI_Wtime();
        rreq_idx = 0; 
        sreq_idx = 0;
        /*Post all receives*/
        for (j = 0; j < msg_count; j++) {
            /*x dim*/
            if (xrankL != -1) { 
                MPI_Irecv((void *)(rbuf + rreq_idx*msg_size), msg_size, 
                        MPI_CHAR, xrankL, 1000*j + xrankL, comm3d, 
                        &rreq[rreq_idx]);
                rreq_idx++;
            }
            if (xrankR != -1) { 
                MPI_Irecv((void *)(rbuf + rreq_idx*msg_size), msg_size, 
                        MPI_CHAR, xrankR, 1000*j + xrankR, comm3d, 
                        &rreq[rreq_idx]);
                rreq_idx++;
            }
            /*y dim*/
            if (yrankL != -1) { 
                MPI_Irecv((void *)(rbuf + rreq_idx*msg_size), msg_size, 
                    MPI_CHAR, yrankL, 1000*j + yrankL, comm3d, 
                    &rreq[rreq_idx]);
                rreq_idx++;
            }
            if (yrankR != -1) { 
                MPI_Irecv((void *)(rbuf + rreq_idx*msg_size), msg_size, 
                    MPI_CHAR, yrankR, 1000*j + yrankR, comm3d, 
                    &rreq[rreq_idx]);
                rreq_idx++;
            }
            /*z dim*/
            if (zrankL != -1) { 
                MPI_Irecv((void *)(rbuf + rreq_idx*msg_size), msg_size, 
                    MPI_CHAR, zrankL, 1000*j + zrankL, comm3d, 
                    &rreq[rreq_idx]);
                rreq_idx++;
            }
            if (zrankR != -1) {  
                MPI_Irecv((void *)(rbuf + rreq_idx*msg_size), msg_size, 
                    MPI_CHAR, zrankR, 1000*j + zrankR, comm3d, 
                    &rreq[rreq_idx]);
                rreq_idx++;
            }
        }
        /*post all sends*/ 
        for (j = 0; j < msg_count; j++) {
            /*x dim*/
            if (xrankL != -1) {
                MPI_Isend((void *)(sbuf + sreq_idx*msg_size), msg_size, 
                        MPI_CHAR, xrankL, 1000*j + rank, comm3d, 
                        &sreq[sreq_idx]);
                sreq_idx++;
            }
            if (xrankR != -1) {
                MPI_Isend((void *)(sbuf + sreq_idx*msg_size), msg_size, 
                        MPI_CHAR, xrankR, 1000*j + rank, comm3d, 
                        &sreq[sreq_idx]);
                sreq_idx++;
            }
            /*y dim*/
            if (yrankL != -1) {
                MPI_Isend((void *)(sbuf + sreq_idx*msg_size), msg_size, 
                        MPI_CHAR, yrankL, 1000*j + rank, comm3d, 
                        &sreq[sreq_idx]);
                sreq_idx++;
            }
            if (yrankR != -1) { 
                MPI_Isend((void *)(sbuf + sreq_idx*msg_size), msg_size, 
                    MPI_CHAR, yrankR, 1000*j + rank, comm3d, 
                    &sreq[sreq_idx]);
                sreq_idx++;
            }
            /*z dim*/
            if (zrankL != -1) { 
                MPI_Isend((void *)(sbuf + sreq_idx*msg_size), msg_size, 
                    MPI_CHAR, zrankL, 1000*j + rank, comm3d, 
                    &sreq[sreq_idx]);
                sreq_idx++;
            }
            if (zrankR != -1) {
                MPI_Isend((void *)(sbuf + sreq_idx*msg_size), msg_size, 
                    MPI_CHAR, zrankR, 1000*j + rank, comm3d, 
                    &sreq[sreq_idx]);
                sreq_idx++;
            }
        }
        MPI_Waitall(rreq_idx, rreq, MPI_STATUS_IGNORE);
        MPI_Waitall(sreq_idx, sreq, MPI_STATUS_IGNORE);

        comm_time = MPI_Wtime() - comm_time;
        if (i > iter_warmup) {
            comm_total += comm_time;
        }
        MPI_Barrier(comm3d);
    }

    MPI_Barrier(comm3d);
    latency = (comm_total * 1e6) / iter_count;
    latency_in_secs = comm_total / iter_count;

    init_arrays();

#if WRITE_OTF2_TRACE
  SCOREP_RECORDING_ON();
  // Marks the beginning of code region to be repeated in simulation
  SCOREP_USER_REGION_BY_NAME_BEGIN("TRACER_Loop", SCOREP_USER_REGION_TYPE_COMMON);
#endif

    /*send and receive messages*/  
    for (i = 0; i < (iter_count + iter_warmup); i++) {
#if WRITE_OTF2_TRACE
  // Marks the beginning of code region to be repeated in simulation
  SCOREP_USER_REGION_BY_NAME_BEGIN("TRACER_Iteration", SCOREP_USER_REGION_TYPE_COMMON);
#endif
        total_time = MPI_Wtime();
        rreq_idx = 0; 
        sreq_idx = 0;
        /*Post all receives*/
        init_time = MPI_Wtime();
        for (j = 0; j < msg_count; j++) {
            /*x dim*/
            if (xrankL != -1) { 
                MPI_Irecv((void *)(rbuf + rreq_idx*msg_size), msg_size, 
                        MPI_CHAR, xrankL, 1000*j + xrankL, comm3d, 
                        &rreq[rreq_idx]);
                rreq_idx++;
            }
            if (xrankR != -1) { 
                MPI_Irecv((void *)(rbuf + rreq_idx*msg_size), msg_size, 
                        MPI_CHAR, xrankR, 1000*j + xrankR, comm3d, 
                        &rreq[rreq_idx]);
                rreq_idx++;
            }
            /*y dim*/
            if (yrankL != -1) { 
                MPI_Irecv((void *)(rbuf + rreq_idx*msg_size), msg_size, 
                    MPI_CHAR, yrankL, 1000*j + yrankL, comm3d, 
                    &rreq[rreq_idx]);
                rreq_idx++;
            }
            if (yrankR != -1) { 
                MPI_Irecv((void *)(rbuf + rreq_idx*msg_size), msg_size, 
                    MPI_CHAR, yrankR, 1000*j + yrankR, comm3d, 
                    &rreq[rreq_idx]);
                rreq_idx++;
            }
            /*z dim*/
            if (zrankL != -1) { 
                MPI_Irecv((void *)(rbuf + rreq_idx*msg_size), msg_size, 
                    MPI_CHAR, zrankL, 1000*j + zrankL, comm3d, 
                    &rreq[rreq_idx]);
                rreq_idx++;
            }
            if (zrankR != -1) {  
                MPI_Irecv((void *)(rbuf + rreq_idx*msg_size), msg_size, 
                    MPI_CHAR, zrankR, 1000*j + zrankR, comm3d, 
                    &rreq[rreq_idx]);
                rreq_idx++;
            }
        }
        /*post all sends*/ 
        for (j = 0; j < msg_count; j++) {
            /*x dim*/
            if (xrankL != -1) {
                MPI_Isend((void *)(sbuf + sreq_idx*msg_size), msg_size, 
                        MPI_CHAR, xrankL, 1000*j + rank, comm3d, 
                        &sreq[sreq_idx]);
                sreq_idx++;
            }
            if (xrankR != -1) {
                MPI_Isend((void *)(sbuf + sreq_idx*msg_size), msg_size, 
                        MPI_CHAR, xrankR, 1000*j + rank, comm3d, 
                        &sreq[sreq_idx]);
                sreq_idx++;
            }
            /*y dim*/
            if (yrankL != -1) {
                MPI_Isend((void *)(sbuf + sreq_idx*msg_size), msg_size, 
                        MPI_CHAR, yrankL, 1000*j + rank, comm3d, 
                        &sreq[sreq_idx]);
                sreq_idx++;
            }
            if (yrankR != -1) { 
                MPI_Isend((void *)(sbuf + sreq_idx*msg_size), msg_size, 
                    MPI_CHAR, yrankR, 1000*j + rank, comm3d, 
                    &sreq[sreq_idx]);
                sreq_idx++;
            }
            /*z dim*/
            if (zrankL != -1) { 
                MPI_Isend((void *)(sbuf + sreq_idx*msg_size), msg_size, 
                    MPI_CHAR, zrankL, 1000*j + rank, comm3d, 
                    &sreq[sreq_idx]);
                sreq_idx++;
            }
            if (zrankR != -1) {
                MPI_Isend((void *)(sbuf + sreq_idx*msg_size), msg_size, 
                    MPI_CHAR, zrankR, 1000*j + rank, comm3d, 
                    &sreq[sreq_idx]);
                sreq_idx++;
            }
        }
        init_time = MPI_Wtime() - init_time;

        comp_time = MPI_Wtime();
#if WRITE_OTF2_TRACE
    // Marks compute region for computation-communication overlap
    SCOREP_USER_REGION_BY_NAME_BEGIN("TRACER_stencil3d_overlap", SCOREP_USER_REGION_TYPE_COMMON);
        dummy_compute(latency_in_secs);
    SCOREP_USER_REGION_BY_NAME_END("TRACER_stencil3d_overlap");
#endif
        comp_time = MPI_Wtime() - comp_time;

        wait_time = MPI_Wtime();
        MPI_Waitall(rreq_idx, rreq, MPI_STATUS_IGNORE);
        MPI_Waitall(sreq_idx, sreq, MPI_STATUS_IGNORE);
        wait_time = MPI_Wtime() - wait_time;

        total_time = MPI_Wtime() - total_time;
        if (i > iter_warmup) {
            init_total += init_time;
            wait_total += wait_time;
            comp_total += comp_time;
            total_total += total_time;
        }

        MPI_Barrier(comm3d);

#if WRITE_OTF2_TRACE
  // Marks the end of code region to be repeated in simulation
  SCOREP_USER_REGION_BY_NAME_END("TRACER_Iteration");
#endif
    }

#if WRITE_OTF2_TRACE
  // Marks the end of code region to be repeated in simulation
  SCOREP_USER_REGION_BY_NAME_END("TRACER_Loop");
  SCOREP_RECORDING_OFF();
#endif

    calculate_and_print_stats(MPI_COMM_WORLD, rank, msg_size, comm_size, iter_count,
                              total_total, latency,
                              comp_total, wait_total, init_total);

    free(sbuf);
    free(rbuf);
    free(sreq);
    free(rreq);

    return 0;
}

int main (int c, char *v[]) 
{
    int comm_size, msg_size, msg_count, iter_count, iter_warmup;
    int px, py, pz; 
    int dim[3], period[3];
    int reorder;
    int rank, xrankL, xrankR, yrankL, yrankR, zrankL, zrankR; 
    MPI_Comm comm3d; 
    double s, f;

    px=2;
    py=2;
    pz=2;
    msg_size=64*1024;
    msg_count=30;
    iter_count=100;
    iter_warmup=2;
 
    if (getenv("PX") != NULL) { 
        px = atoi(getenv("PX")); 
    }
    if (getenv("PY") != NULL) {
        py = atoi(getenv("PY"));
    }
    if (getenv("PZ") != NULL) {
        pz = atoi(getenv("PZ"));
    }
    if (getenv("MSG_SIZE") != NULL) { 
        msg_size = atoi(getenv("MSG_SIZE"));
    }
    if (getenv("MSG_COUNT") != NULL) {
        msg_count = atoi(getenv("MSG_COUNT"));
    }
    if (getenv("ITER_COUNT") != NULL) {
        iter_count = atoi(getenv("ITER_COUNT"));
    }

    px = atoi(v[1]);
    py = atoi(v[2]);
    pz = atoi(v[3]);
    iter_count = atoi(v[4]);

    MPI_Init(&c, &v);

#if WRITE_OTF2_TRACE
  SCOREP_RECORDING_OFF();
#endif

    MPI_Comm_size(MPI_COMM_WORLD, &comm_size); 
    
    if (comm_size != px*py*pz) {
        fprintf(stderr, "comm size and number of processes do not match \n");
        fprintf(stderr, "comm_size = %d, px = %d, py = %d, pz = %d\n",
                    comm_size, px, py, pz);
        exit(-1);
    } 
   
    /*create the stencil communicator*/ 
    dim[0]    = px;
    dim[1]    = py;
    dim[2]    = pz;
    period[0] = 0;
    period[1] = 0;
    period[2] = 0;
    reorder = 1; 

    MPI_Cart_create (MPI_COMM_WORLD, 3, dim, period, reorder, &comm3d);
    MPI_Cart_shift(comm3d, 0,  1,  &xrankL, &xrankR );
    MPI_Cart_shift(comm3d, 1,  1,  &yrankL, &yrankR );
    MPI_Cart_shift(comm3d, 2,  1,  &zrankL, &zrankR );
    MPI_Comm_rank(comm3d, &rank);

    if (rank == 0) {
        fprintf(stdout, "PX: %d PY: %d PZ: %d msg_size: %d msg_count: %d iter_count: %d \n", 
                px, py, pz, msg_size, msg_count, iter_count);
    }

    if(!rank)
        print_preamble_nbc(rank);

    MPI_Barrier(MPI_COMM_WORLD);

    s = MPI_Wtime(); 
    //for (msg_size = 1; msg_size < 1<<20; msg_size=msg_size*2) {
        msg_size = 131072;
        MPI_Barrier(MPI_COMM_WORLD);
        sendrecv(comm3d, msg_size, msg_count, iter_count, iter_warmup,
                rank, comm_size, xrankL, xrankR, yrankL, yrankR, zrankL, zrankR);
    //}
    
    MPI_Barrier(MPI_COMM_WORLD);

    if(!rank)
        print_preamble_nbc(rank);

    f = MPI_Wtime() - s;
    if(rank == 0) 
      fprintf(stderr, "Total time: %lf\n", f);

    MPI_Comm_free (&comm3d);
    MPI_Finalize();

    return 0;
}
