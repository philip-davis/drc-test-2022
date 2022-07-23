module load spack
module load rdma-credentials

spack load mochi-margo

make

sbatch run.sh
