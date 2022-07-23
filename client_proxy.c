/*
 * Copyright (c) 2016 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <mpi.h>

#include <margo.h>
#include <rdmacred.h>

#define DIE_IF(cond_expr, err_fmt, ...) \
    do { \
        if (cond_expr) { \
            fprintf(stderr, "ERROR at %s:%d (" #cond_expr "): " \
                    err_fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
            exit(1); \
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
    char config[1024];
    struct margo_init_info args = {0};
    int rank;
    FILE *fd;
    int ret;

    if(argc != 2) {
        fprintf(stderr, "don't forget the connection string (e.g. verbs, gni, ...)\n");
    }

    /* set margo config json string */
    snprintf(config, 1024,
             "{ \"use_progress_thread\" : %s, \"rpc_thread_count\" : %d }",
             "false", -1);

    memset(&hii, 0, sizeof(hii));

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(rank == 0) {
        fd = fopen("drcid", "r");
        if(!fd) {
            fprintf(stderr, "could not open drcid\n");
        }
        fscanf(fd, "%" PRIu32, &drc_credential_id);
        fclose(fd);
    }
    MPI_Bcast(&drc_credential_id, 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);   
 
    /* access credential and covert to string for use by mercury */
    ret = drc_access(drc_credential_id, 0, &drc_credential_info);
    DIE_IF(ret != DRC_SUCCESS, "drc_access %u", drc_credential_id);
    drc_cookie = drc_get_first_cookie(drc_credential_info);
    sprintf(drc_key_str, "%u", drc_cookie);
    hii.na_init_info.auth_key = drc_key_str;

    /* init margo */
    /* use the main xstream to drive progress & run handlers */
    mid = margo_init_ext(argv[1], MARGO_CLIENT_MODE, &args);
    DIE_IF(mid == MARGO_INSTANCE_NULL, "margo_init");

    margo_finalize(mid);

    MPI_Finalize();

    return 0;
}
