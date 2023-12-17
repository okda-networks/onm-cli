//
// Created by ali on 11/19/23.
//
#include <signal.h>
#include "onm_sysrepo.h"


static sr_conn_ctx_t *connection = NULL;
static sr_session_ctx_t *session = NULL;

// FWD decleration for disconnect
int sysrepo_disconnect();

void cleanup_handler(int signo) {
    printf("Received signal %d. Cleaning up...\n", signo);
    sysrepo_disconnect();


    exit(EXIT_SUCCESS);
}

void my_log_cb(sr_log_level_t level, const char *message) {
    printf("Sysrepo log [%d]: %s\n", level, message);
}

int sysrepo_connect() {
    if (sr_connect(SR_CONN_DEFAULT, &connection) != SR_ERR_OK) {
        fprintf(stderr, "Failed to connect to Sysrepo\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int sysrepo_disconnect() {
    if (sr_disconnect(connection) != SR_ERR_OK) {
        fprintf(stderr, "Failed to disconnect from Sysrepo\n");
        return EXIT_FAILURE;
    }
    fprintf(stdout, "disconnect from sysrepo successfully\n");
    return EXIT_SUCCESS;

}

int sysrepo_start_session() {
    // Start a new session
    if (sr_session_start(connection, SR_DS_RUNNING, &session) != SR_ERR_OK) {
        fprintf(stderr, "Failed to start a new Sysrepo session\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

const struct ly_ctx *sysrepo_get_ctx() {
    return sr_acquire_context(connection);
}

sr_session_ctx_t *sysrepo_get_session() {
    return session;
}

int sysrepo_release_ctx() {
    sr_release_context(connection);
    return EXIT_SUCCESS;
}

int sysrepo_discard_changes() {
    return sr_discard_changes(session);
}


int sysrepo_has_uncommited_changes(struct lyd_node *data_node) {
    // get the respective data_node from sysrepo, and compare it with current data_node.
    // 0 no changes, 1 there is changes.
    char xpath[256];
    memset(xpath, '\0', 256);
    lyd_path(data_node, LYD_PATH_STD, xpath, 256);
    sr_data_t *sysrepo_subtree;
    int ret = sr_get_subtree(sysrepo_get_session(), xpath, 0, &sysrepo_subtree);
    if (ret == SR_ERR_OK) {
        struct lyd_node *diff;
        lyd_diff_tree(data_node, sysrepo_subtree->tree, 0, &diff);
        if (diff == NULL)
            return 0;
        else
            return 1;
    }
    if (ret == SR_ERR_NOT_FOUND)
        return 1;
}

int sysrepo_commit(struct lyd_node *data_tree) {
    // Check if there is data_tree to add and apply
    if (data_tree != NULL) {
        // If there are changes in the session, add the data_tree using sr_edit_batch
        if (sr_edit_batch(session, data_tree, "replace") != SR_ERR_OK) {
            fprintf(stderr, "Failed to add data_tree to Sysrepo changes\n");
            return EXIT_FAILURE;
        }
        // Apply the changes (if any)
        if (sr_apply_changes(session, 0) != SR_ERR_OK) {
            fprintf(stderr, "Failed to commit changes to Sysrepo\n");
            sr_discard_changes(session);
            return EXIT_FAILURE;
        }

    }
    return EXIT_SUCCESS;
}

int sysrepo_init() {
    sr_log_stderr(SR_LL_DBG);

    // Register logging callback
    sr_log_set_cb(my_log_cb);

    // Set up signal handler for SIGINT (Ctrl+C)
    if (signal(SIGINT, cleanup_handler) == SIG_ERR) {
        perror("Unable to set up signal handler for SIGINT");
        return EXIT_FAILURE;
    }
    if (signal(SIGTERM, cleanup_handler) == SIG_ERR) {
        perror("Unable to set up signal handler");
        return EXIT_FAILURE;
    }

    if (sysrepo_connect() != EXIT_SUCCESS)
        return EXIT_FAILURE;

    if (sysrepo_start_session() != EXIT_SUCCESS)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;

}