/* SPDX-License-Identifier: AGPL-3.0-or-later */
/*
 * Authors:     Ali Aqrabawi, <aaqrbaw@okdanetworks.com>
 *
 *              This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU Affero General Public
 *              License Version 3.0 as published by the Free Software Foundation;
 *              either version 3.0 of the License, or (at your option) any later
 *              version.
 *
 * Copyright (C) 2024 Okda Networks, <aaqrbaw@okdanetworks.com>
 */

#include <signal.h>
#include "onm_sysrepo.h"
#include "onm_logger.h"

static sr_conn_ctx_t *connection = NULL;
static sr_session_ctx_t *session = NULL, *running_session = NULL, *startup_session = NULL, *operational_session = NULL;


char *module_path = NULL;

// schema-mount args (optional, set from CLI)
static char *schema_mount_module = NULL;
static char *schema_mount_label = NULL;


// forward declaration
int sysrepo_disconnect();

void sysrepo_set_module_path(char *path) {
    if (module_path != NULL)
        free(module_path);
    module_path = malloc(strlen(path) + 1);
    if (module_path) {
        memcpy(module_path, path, strlen(path) + 1);
    }
}

void sysrepo_set_schema_mount_args(const char *module, const char *label) {
    if (schema_mount_module) { free(schema_mount_module); schema_mount_module = NULL; }
    if (schema_mount_label) { free(schema_mount_label); schema_mount_label = NULL; }
    if (module) {
        schema_mount_module = malloc(strlen(module) + 1);
        memcpy(schema_mount_module, module, strlen(module) + 1);
    }
    if (label) {
        schema_mount_label = malloc(strlen(label) + 1);
        memcpy(schema_mount_label, label, strlen(label) + 1);
    }
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
    // Start a edit session
    if (sr_session_start(connection, SR_DS_RUNNING, &session) != SR_ERR_OK) {
        LOG_ERROR("Failed to start a new Sysrepo session");
        return EXIT_FAILURE;
    }
    // start running session
    if (sr_session_start(connection, SR_DS_RUNNING, &running_session) != SR_ERR_OK) {
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

// helper: populate using given session, restore DS if not operational_session
static int populate_schema_mount_in_session(sr_session_ctx_t *schema_session) {
    if (!schema_session) return EXIT_SUCCESS;

    int rc;
    sr_datastore_t orig_ds = sr_session_get_ds(schema_session);

    rc = sr_session_switch_ds(schema_session, SR_DS_OPERATIONAL);
    if (rc != SR_ERR_OK) {
        LOG_ERROR("Error: sr_session_switch_ds to operational failed: %s", sr_strerror(rc));
        return EXIT_FAILURE;
    }

    char path[512];
    snprintf(path, sizeof(path),
             "/ietf-yang-schema-mount:schema-mounts/mount-point[module='%s'][label='%s']/shared-schema",
             schema_mount_module, schema_mount_label);

    rc = sr_set_item_str(schema_session, path, NULL, NULL, 0);
    if (rc != SR_ERR_OK) {
        LOG_ERROR("Error: sr_set_item_str failed to set config: %s", sr_strerror(rc));
        // try to restore
        if (schema_session != operational_session && orig_ds != SR_DS_OPERATIONAL) {
            sr_session_switch_ds(schema_session, orig_ds);
        }
        return EXIT_FAILURE;
    }

    rc = sr_apply_changes(schema_session, 0);
    if (rc != SR_ERR_OK) {
        LOG_ERROR("Error: sr_apply_changes failed: %s", sr_strerror(rc));
        if (schema_session != operational_session && orig_ds != SR_DS_OPERATIONAL) {
            sr_session_switch_ds(schema_session, orig_ds);
        }
        return EXIT_FAILURE;
    }

    if (schema_session != operational_session && orig_ds != SR_DS_OPERATIONAL) {
        rc = sr_session_switch_ds(schema_session, orig_ds);
        if (rc != SR_ERR_OK) {
            LOG_ERROR("Error: sr_session_switch_ds restore failed: %s", sr_strerror(rc));
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

// Populate ietf-yang-schema-mount mount-point with provided module/label
int sysrepo_populate_schema_mounts() {
    if (!schema_mount_module || !schema_mount_label) {
        // Nothing to populate if args are not provided
        return EXIT_SUCCESS;
    }

    int ret = EXIT_SUCCESS;
    if (populate_schema_mount_in_session(session) != EXIT_SUCCESS) ret = EXIT_FAILURE;
    if (populate_schema_mount_in_session(running_session) != EXIT_SUCCESS) ret = EXIT_FAILURE;
    if (populate_schema_mount_in_session(startup_session) != EXIT_SUCCESS) ret = EXIT_FAILURE;
    if (populate_schema_mount_in_session(operational_session) != EXIT_SUCCESS) ret = EXIT_FAILURE;

    if (ret == EXIT_SUCCESS) {
        LOG_INFO("schema-mount populated: module=%s label=%s", schema_mount_module, schema_mount_label);
    }
    return ret;
}


const struct ly_ctx *sysrepo_get_ctx() {
    LOG_DEBUG("sysrepo context acquired!");
    return sr_acquire_context(connection);
}


sr_session_ctx_t *sysrepo_get_session() {
    return session;
}

sr_session_ctx_t *sysrepo_get_running_session() {
    return running_session;
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


struct lyd_node *sysrepo_get_data_subtree(const char *path) {
    sr_data_t *sr_data;
    int ret = sr_get_subtree(session, path, 0, &sr_data);
    if (ret != SR_ERR_OK || sr_data == NULL)
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

int sysrepo_commit() {
    // Apply the changes (if any)
    if (sr_apply_changes(session, 0) != SR_ERR_OK) {
        LOG_ERROR("Failed to commit changes to Sysrepo");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int onm_sysrepo_done() {
    sysrepo_disconnect();
    return EXIT_SUCCESS;
}

int onm_sysrepo_init() {
    sr_log_stderr(SR_LL_INF);

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

    // If CLI provided schema-mount args, populate across sessions
    if (sysrepo_populate_schema_mounts() != EXIT_SUCCESS) {
        LOG_ERROR("failed to populate schema-mounts during init");
        // continue even on failure
    }



    return EXIT_SUCCESS;

}
