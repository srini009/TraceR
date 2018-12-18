#!/bin/bash

#module load intel

data_column=2
MANUAL=0
num_iters=1
ppn=28
MPI_ROOT=/home/sramesh/SOFTWARE/MVAPICH_INSTALLATION

export PATH=$(pwd):$MPI_ROOT/bin:$PATH
export LD_LIBRARY_PATH=$MPI_ROOT/lib:$LD_LIBRARY_PATH

if [ "$MANUAL" == "0" ]
then
    hostfile=hosts-$SLURM_JOBID
    rm -f $hostfile
    
    for i in `scontrol show hostnames $SLURM_NODELIST`
    do
      for (( j=0; j<$ppn; j++ ))
      do
        echo $i>>$hostfile
      done
    done
    
    nprocs=`wc -l $hostfile | awk '{ print $1 }'`
else
    SLURM_JOBID=whoami
    hostfile=hosts-$SLURM_JOBID
fi

common="MV2_IBA_HCA=mlx5_0 MV2_ENABLE_AFFINITY=1 PATH=$PATH LD_LIBRARY_PATH=$LD_LIBRARY_PATH"

combo=(
    ""
    "MV2_RNDV_PROTOCOL=RGET"
)


combo_name=(
    "Default"
    "RGET"
)

#Find out number of env combos to run
num_combos=${#combo[@]}
num_combos=`echo "$num_combos - 1" | bc`

if [ "$nprocs" == "56" ]
then
    export PX=4
    export PY=7
    export PZ=2
    export iter_count=1000
elif [ "$nprocs" == "32" ]
then
    export PX=4
    export PY=4
    export PZ=2
    export iter_count=500
elif [ "$nprocs" == "112" ]
then
    export PX=4
    export PY=7
    export PZ=4
    export iter_count=500
elif [ "$nprocs" == "224" ]
then
    export PX=4
    export PY=7
    export PZ=8
    export iter_count=1000
elif [ "$nprocs" == "448" ]
then
    export PX=4
    export PY=7
    export PZ=16
    export iter_count=500
else
    exit 1
fi

export XWT=1
export YWT=1
export ZWT=1
export WEIGHT=1
export NX=256
bench_args="$PX $PY $PZ $iter_count $NX $XWT $YWT $ZWT"

for test in 3Dstencil
do
    for combo_num in `seq 0 $num_combos`
    do
	    #Run test
        echo "========================================="
        echo "Running combination ${combo_name[$combo_num]}"
        echo "========================================="
        echo "Running with "$nprocs" processes"

        date1=$(($(date +%s%N)/1000000))

        echo "Running $test with ${combo_name[$combo_num]} - $common & ${combo[$combo_num]}"
	    for i in `seq 1 $num_iters`
	    do
	        $MPI_ROOT/bin/mpirun_rsh -np $nprocs -hostfile $hostfile $common ${combo[$combo_num]} ./$test $bench_args &> run-$test-${combo_name[$combo_num]}-$SLURM_JOBID-$i
            sleep 1
	    done
        date2=$(($(date +%s%N)/1000000))
        diff=$(($date2-$date1))
        echo "Total Wall Time for $num_iters iterations: $date2  -  $date1 = $diff millisecs  <======"
        echo "#########################################"
    done
done

#Cleanup
rm -f $hostfile

