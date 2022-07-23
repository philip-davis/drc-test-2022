/*
 * Copyright (c) 2016 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */


#define _GNU_SOURCE
#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include <margo.h>
#include <rdmacred.h>

#define DIE_IF(cond_expr, err_fmt, ...) \
    do { \
        if (cond_expr) { \
            fprintf(stderr, "ERROR at %s:%d (" #cond_expr "): " \
                    err_fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

int main(int argc, char *argv[])
{
    margo_instance_id mid = MARGO_INSTANCE_NULL;
    struct hg_init_info hii;
    uint32_t drc_credential_id;
    drc_info_handle_t drc_credential_info;
    uint32_t drc_cookie;
    char drc_key_str[256] = {0};
    int rank;
    char config[1024];
    struct margo_init_info args = {0};
    FILE *fd;
    int ret;

    if(argc != 2) {
        fprintf(stderr, "don't forget connection string (e.g. verbs, gni, ...)\n");
        return(1);
    }

    /* set margo config json string */
    snprintf(config, 1024,
             "{ \"use_progress_thread\" : %s, \"rpc_thread_count\" : %d }",
             "false", -1);


    memset(&hii, 0, sizeof(hii));

    int mpi_rank, mpi_size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    /* acquire DRC cred on MPI rank 0 */
    if (mpi_rank == 0)
    {
        ret = drc_acquire(&drc_credential_id, DRC_FLAGS_FLEX_CREDENTIAL);
        DIE_IF(ret != DRC_SUCCESS, "drc_acquire");
        fd = fopen("drcid", "w");
        if(!fd) 
        {
            fprintf(stderr, "ERROR: couldn't open 'drcid' for writing.\n");
            return(1);
            
        } 
        fprintf(fd, "%" PRIu32 "\n", drc_credential_id);
        fclose(fd); 
    } 
    MPI_Bcast(&drc_credential_id, 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);
    rank = mpi_rank;

    /* access credential on all ranks and convert to string for use by mercury */
    ret = drc_access(drc_credential_id, 0, &drc_credential_info);
    DIE_IF(ret != DRC_SUCCESS, "drc_access");
    drc_cookie = drc_get_first_cookie(drc_credential_info);
    sprintf(drc_key_str, "%u", drc_cookie);
    hii.na_init_info.auth_key = drc_key_str;

    /* rank 0 grants access to the credential, allowing other jobs to use it */
    if(rank == 0)
    {
        ret = drc_grant(drc_credential_id, drc_get_wlm_id(), DRC_FLAGS_TARGET_WLM);
        DIE_IF(ret != DRC_SUCCESS, "drc_grant");
    }

    /* init margo */
    /* use the main xstream to drive progress & run handlers */
    args.json_config = config;
    args.hg_init_info = &hii;
    mid = margo_init_ext(argv[1], MARGO_SERVER_MODE, &args);
    DIE_IF(mid == MARGO_INSTANCE_NULL, "margo_init");

    margo_finalize(mid);
        
    MPI_Finalize();

    return 0;
}
