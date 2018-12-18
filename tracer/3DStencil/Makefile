ROOT=/home/sramesh/SOFTWARE/MVAPICH_INSTALLATION
CC=scorep --user --nocompiler --noopenmp --nopomp --nocuda --noopenacc --noopencl --nomemory ${ROOT}/bin/mpicc
CXX=scorep --user --nocompiler --noopenmp --nopomp --nocuda --noopenacc --noopencl --nomemory ${ROOT}/bin/mpicxx

all: clean
	${CC} -DWRITE_OTF2_TRACE=1 -DSCOREP_USER_ENABLE -I/home/sramesh/NONBLOCKING_EXPERIMENTS/SCOREP/include/scorep -c -g 3Dstencil_overlap.c ${LIBS}
	${CC} -o 3Dstencil  3Dstencil_overlap.o ${LIBS}
clean :
	rm -f 3Dstencil; rm -f *.o; rm -f *core*
