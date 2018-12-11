ROOT=/home/sramesh/SOFTWARE/MVAPICH_INSTALLATION
CC=${ROOT}/bin/mpicc
CXX=${ROOT}/bin/mpicxx

all: clean
	${CC} -c -g 3Dstencil_overlap.c ${LIBS}
	${CC} -o 3Dstencil  3Dstencil_overlap.o ${LIBS}
clean :
	rm -f 3Dstencil; rm -f *.o; rm -f *core*
