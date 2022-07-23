#!/bin/bash
#SBATCH --qos=debug
#SBATCH --time=5
#SBATCH --nodes=9
#SBATCH --ntasks-per-node=32
#SBATCH --constraint=haswell

rm -f drcid
      
fi_info -l

export FI_LOG_LEVEL=warn
 
num_procs=256
procs_per_node=32
(( num_nodes=($num_procs + $procs_per_node - 1)/$procs_per_node )) # ceil(num_procs / procs_per_node)
srv_procs=1
srv_nodes=1

# prevents interference between MPI and Mochi (libfabric) libraries when sharing GNI resources
# export MPICH_GNI_NDREG_ENTRIES=1024
export MPICH_GNI_NDREG_ENTRIES=512

echo -e "Starting server with $srv_procs procs and $srv_nodes nodes\n" 1>&2
# start dataspaces server and wait until it's ready
srun -N $srv_nodes -n $srv_procs ./server gni &
echo -e "server running; waiting for drcid file to appear\n" 1>&2
while [ ! -f drcid ]; do
    sleep 1s
done
sleep 5 # extra safety, might not be needed
echo -e "Starting experiment with $num_procs procs and $num_nodes nodes\n" 1>&2
# run the experiment
srun --label -r 1 -N $num_nodes -n $num_procs ./client gni &
# wait for all sruns to finish
wait
retval=$?
if [ $retval == 0 ]; then
    exit 0
else
    exit 1
fi
