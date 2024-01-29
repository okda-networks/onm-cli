//
// Created by ali on 11/19/23.
//
#include <signal.h>
#include "onm_sysrepo.h"
#include "onm_logger.h"

static sr_conn_ctx_t *connection = NULL;
static sr_session_ctx_t *session = NULL, *startup_session = NULL,*operational_session = NULL;

struct data_tree *config_root_tree;

char *module_path = NULL;

void free_data_tree(struct data_tree *dtree) {
    lyd_free_all(dtree->node);
    dtree->prev = NULL;
    free(dtree);
}

void free_data_tree_all() {
    struct data_tree *curr_node = config_root_tree;
    while (curr_node != NULL) {
        struct data_tree *next_node = curr_node->prev;  // Save the pointer to the next node
        lyd_free_all(curr_node->node);
        free(curr_node);
        curr_node = next_node;  // Move to the next node
    }
    config_root_tree = NULL;
}

// forward declaration
int sysrepo_disconnect();

void sysrepo_set_module_path(char *path) {
    if (module_path != NULL)
        free(module_path);
    module_path = malloc(sizeof(char) * (strlen(path) + 1));
    memcpy(module_path, path, strlen(path));
    return;
}


int sysrepo_insmod(char *mod) {
    if (module_path == NULL) {
        printf("[ERR] please set module path: # sysrepo set-module-path /path/to/module\n");
        return EXIT_FAILURE;
    }
    char *mod_path = malloc(sizeof(char) * (strlen(mod) + strlen(module_path) + 2));
    sprintf(mod_path, "%s/%s", module_path, mod);


    int ret = sr_install_module(connection, mod_path, module_path, NULL);
    free(mod_path);
    if (ret != SR_ERR_OK)
        return EXIT_FAILURE;
    LOG_INFO("module %s installed in sysrepo", mod);

    return EXIT_SUCCESS;
}

int sysrepo_rmmod(char *mod, int force) {
    if (sr_remove_module(connection, mod, force) != SR_ERR_OK)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}


void cleanup_handler(int signo) {
    LOG_INFO("Received signal %d. Cleaning up...", signo);
    sysrepo_disconnect();


    exit(EXIT_SUCCESS);
}

//static void print_errs(struct cli_def *cli) {
//    const sr_error_info_t *sysrepo_err;
//    sr_session_get_error(session, &sysrepo_err);
//    if (sysrepo_err != NULL) {
//        for (int i = 0; i < sysrepo_err->err_count; i++) {
//            cli_print(cli, "SYSREPO: %s", sysrepo_err->err[i].message);
//        }
//    }
//}

int sysrepo_connect() {
    if (sr_connect(SR_CONN_DEFAULT, &connection) != SR_ERR_OK) {
        LOG_ERROR("Failed to connect to Sysrepo");
        return EXIT_FAILURE;
    }
    LOG_INFO("connection to sysrepo closed!");
    return EXIT_SUCCESS;
}

int sysrepo_disconnect() {
    if (sr_disconnect(connection) != SR_ERR_OK) {
        LOG_ERROR("Failed to disconnect from Sysrepo");
        return EXIT_FAILURE;
    }
    LOG_INFO("disconnect from sysrepo successfully");
    return EXIT_SUCCESS;

}

int sysrepo_start_session() {
    // Start a new session
    if (sr_session_start(connection, SR_DS_RUNNING, &session) != SR_ERR_OK) {
        LOG_ERROR("Failed to start a new Sysrepo session");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int sysrepo_start_session_startup() {
    // Start a new session
    if (sr_session_start(connection, SR_DS_STARTUP, &startup_session) != SR_ERR_OK) {
        LOG_ERROR("Failed to start a new Sysrepo session");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int sysrepo_start_session_operational() {
    // Start a new session
    if (sr_session_start(connection, SR_DS_OPERATIONAL, &operational_session) != SR_ERR_OK) {
        LOG_ERROR("Failed to start a new Sysrepo session");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


const struct ly_ctx *sysrepo_get_ctx() {
    LOG_DEBUG("sysrepo context acquired!");
    return sr_acquire_context(connection);
}


sr_session_ctx_t *sysrepo_get_session() {
    return session;
}

sr_session_ctx_t *sysrepo_get_session_startup() {
    if (startup_session == NULL)
        if (sysrepo_start_session_startup() != SR_ERR_OK)
            LOG_ERROR("failed to start session to startup DS.");
    return startup_session;
}

sr_session_ctx_t *sysrepo_get_session_operational() {
    if (operational_session == NULL)
        if (sysrepo_start_session_operational() != SR_ERR_OK)
            LOG_ERROR("failed to start session to operational DS.");
    return operational_session;
}

struct lyd_node *sysrepo_get_data_subtree(struct lysc_node *y_node) {
    sr_data_t *sr_data;
    char xpath[1028];
    lysc_path(y_node, LYSC_PATH_DATA, xpath, 256);
    int ret = sr_get_subtree(session, xpath, 0, &sr_data);
    if (ret != SR_ERR_OK)
        return NULL;
    return sr_data->tree;
}

int sysrepo_release_ctx() {
    LOG_DEBUG("sysrepo context released!");
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
        sr_release_data(sysrepo_subtree);
        if (diff != NULL)
            return 1;
    }
    if (ret == SR_ERR_NOT_FOUND)
        return 1;
    return 0;
}

int sysrepo_commit(struct lyd_node *data_tree) {
    // Check if there is data_tree to add and apply

    if (data_tree != NULL) {
        // If there are changes in the session, add the data_tree using sr_edit_batch
        if (sr_edit_batch(session, data_tree, "replace") != SR_ERR_OK) {
            LOG_ERROR("Failed to add data_tree to Sysrepo changes");
            return EXIT_FAILURE;
        }
        // Apply the changes (if any)
        if (sr_apply_changes(session, 0) != SR_ERR_OK) {
//            print_errs(cli);
            LOG_ERROR("Failed to commit changes to Sysrepo");
            sr_discard_changes(session);
            return EXIT_FAILURE;
        }

    }
    return EXIT_SUCCESS;
}

int onm_sysrepo_done() {
    free_data_tree_all();
    sysrepo_disconnect();
    return EXIT_SUCCESS;
}

int onm_sysrepo_init() {
    sr_log_stderr(SR_LL_ERR);

    // Set up signal handler for SIGINT (Ctrl+C)
    if (signal(SIGINT, cleanup_handler) == SIG_ERR) {
        LOG_ERROR("Unable to set up signal handler for SIGINT");
        return EXIT_FAILURE;
    }
    if (signal(SIGTERM, cleanup_handler) == SIG_ERR) {
        LOG_ERROR("Unable to set up signal handler");
        return EXIT_FAILURE;
    }

    if (sysrepo_connect() != EXIT_SUCCESS)
        return EXIT_FAILURE;

    if (sysrepo_start_session() != EXIT_SUCCESS)
        return EXIT_FAILURE;

    if (sysrepo_start_session_startup() != EXIT_SUCCESS)
        return EXIT_FAILURE;

    if (sysrepo_start_session_operational() != EXIT_SUCCESS)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;

}