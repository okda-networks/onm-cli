//
// Created by ali on 11/19/23.
//
#include "onm_sysrepo.h"

static sr_conn_ctx_t *connection = NULL;
static sr_session_ctx_t *session = NULL;

void my_log_cb(sr_log_level_t level, const char *message) {
    printf("Sysrepo log [%d]: %s\n", level, message);
}

int sysrepo_connect(){
    if (sr_connect(SR_CONN_DEFAULT, &connection) != SR_ERR_OK) {
        fprintf(stderr, "Failed to connect to Sysrepo\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int sysrepo_disconnect(){
    if (sr_connect(SR_CONN_DEFAULT, &connection) != SR_ERR_OK) {
        fprintf(stderr, "Failed to disconnect from Sysrepo\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int sysrepo_start_session(){
    // Start a new session
    if (sr_session_start(connection, SR_DS_RUNNING,  &session) != SR_ERR_OK) {
        fprintf(stderr, "Failed to start a new Sysrepo session\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

const struct ly_ctx * sysrepo_get_ctx(){
    return sr_acquire_context(connection);
}

int sysrepo_release_ctx(){
    sr_release_context(connection);
    return EXIT_SUCCESS;
}

int sysrepo_commit(struct lyd_node *data_tree){

    if (sr_edit_batch(session, data_tree, "merge") != SR_ERR_OK) {
        fprintf(stderr, "Failed to write the data to Sysrepo\n");
        return EXIT_FAILURE;
    }

    if (sr_apply_changes(session, 0) != SR_ERR_OK) {
        fprintf(stderr, "Failed to commit changes to Sysrepo\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int sysrepo_init(){
    sr_log_stderr(SR_LL_DBG);

    // Register logging callback
    sr_log_set_cb(my_log_cb);

    if (sysrepo_connect() != EXIT_SUCCESS)
        return EXIT_FAILURE;

    if (sysrepo_start_session() != EXIT_SUCCESS)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;

}