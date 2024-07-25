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

#include "src/utils.h"
#include "default_cmd.h"
#include "src/onm_sysrepo.h"
#include "yang_core/data_factory.h"
#include "src/onm_logger.h"
#include "yang_core/data_print.h"

#ifdef __GNUC__
#define UNUSED(d) d __attribute__((unused))
#else
#define UNUSED(d) d
#endif

extern struct lyd_node *parent_data;


unsigned int regular_count = 0;
unsigned int debug_regular = 0;

enum {
    RUNNING_DATASTORE,
    CANDIDATE_DATASTORE,
    STARTUP_DATASTORE,
};

int cmd_regular_callback(struct cli_def *cli) {
    regular_count++;
    if (debug_regular) {
        cli_print(cli, "Regular callback - %u times so far", regular_count);
        cli_reprompt(cli);
    }
    return CLI_OK;
}

int cmd_no(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    cli_print(cli, "incomplete command!");
    return CLI_ERROR;
}

int cmd_discard_changes(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {


    if (sr_has_changes(sysrepo_get_session())) {
        if (sysrepo_discard_changes() != SR_ERR_OK) {
            cli_print(cli, RED" failed to discard changes, sysrepo error!" RESET);
            return CLI_ERROR;
        }
        cli_print(cli, GREEN" changes discarded!"RESET);
    } else
        cli_print(cli, " no changes to discard!");
    free_parent_data();
    cli_set_configmode(cli, MODE_CONFIG, NULL);
    return CLI_OK;
}

int cmd_exit2(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (cli->mode == MODE_CONFIG) {
        if (sr_has_changes(sysrepo_get_session())) {
            cli_print(cli,
                      YELLOW" there are uncommitted changes, please `commit` or `discard-changes` before exist!"RESET);
            return CLI_ERROR;
        }

        sysrepo_release_ctx();
        return cli_exit(cli, c, cmd, argv, argc);
    }
    // we need to shift the parent_data backward with each exit call.
    if (parent_data != NULL) {
        struct lyd_node *prev_parent = (struct lyd_node *) parent_data->parent;
        while (prev_parent != NULL) {
            if (prev_parent->schema->nodetype == LYS_LIST || is_root_node(prev_parent->schema))
                break;
            prev_parent = (struct lyd_node *) prev_parent->parent;
        }
        parent_data = (struct lyd_node *) prev_parent;
    }

    return cli_exit(cli, c, cmd, argv, argc);
}

int core_cmd_config_printer(struct cli_def *cli, int datastore) {

    struct ly_ctx *sysrepo_ctx = (struct ly_ctx *) sysrepo_get_ctx();
    struct lys_module *mod;
    unsigned int index = 0;


    while ((mod = (struct lys_module *) ly_ctx_get_module_iter(sysrepo_ctx, &index))) {
        if (mod != NULL) {
            if (!mod->implemented) {
                continue;
            }
            if (mod->compiled->data == NULL)
                continue;
            char xpath[1024] = {0};
            lysc_path(mod->compiled->data, LYSC_PATH_DATA, xpath, 1024);
            struct lyd_node *dnode;
            switch (datastore) {
                case CANDIDATE_DATASTORE:
                    dnode = get_sysrepo_candidate_node(xpath);
                    break;
                case RUNNING_DATASTORE:
                    dnode = get_sysrepo_running_node(xpath);
                    break;
                case STARTUP_DATASTORE:
                    dnode = get_sysrepo_startup_node(xpath);
                    break;

                default:
                    dnode = get_sysrepo_candidate_node(xpath);
                    break;
            }

            if (dnode == NULL)
                continue;
            char *result;
            config_print_mem(&result, dnode);
            cli_print(cli, "%s", result);
            lyd_free_all(dnode);
            free(result);
        }
    }
    sysrepo_release_ctx();
    return CLI_OK;
}

int cmd_show_config_running(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    return core_cmd_config_printer(cli, RUNNING_DATASTORE);
}

int cmd_show_config_startup(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    return core_cmd_config_printer(cli, STARTUP_DATASTORE);
}


int cmd_show_config_candidate(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    return core_cmd_config_printer(cli, CANDIDATE_DATASTORE);
}

int cmd_commit(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (sr_has_changes(sysrepo_get_session()) == 0) {
        cli_print(cli, " no modification to commit!");
    } else {
        if (sysrepo_commit() == EXIT_SUCCESS)
            cli_print(cli, GREEN" commit: changes applied successfully!"RESET);
        else
            cli_print(cli, RED" commit: failed to commit changes!"RESET);
    }

    return CLI_OK;
}


int default_commands_init(struct cli_def *cli) {
    LOG_INFO("commands.c: initializing commands\n");

    cli_register_command(cli, NULL, NULL,
                         "exit", cmd_exit2, PRIVILEGE_UNPRIVILEGED,
                         MODE_ANY, NULL, "exit to prev mode");

    cli_register_command(cli, NULL, NULL,
                         "commit", cmd_commit, PRIVILEGE_UNPRIVILEGED,
                         MODE_ANY, NULL, "commit changes to sysrepo cdb");


    struct cli_command *show = cli_register_command(cli, NULL, NULL,
                                                    "show", NULL, PRIVILEGE_UNPRIVILEGED,
                                                    MODE_ANY, NULL, "print the candidate/running config");


    struct cli_command *config_candidate = cli_register_command(cli, show, NULL,
                                                                "config-candidate", cmd_show_config_candidate,
                                                                PRIVILEGE_UNPRIVILEGED,
                                                                MODE_ANY, "config-candidate",
                                                                "show the candidate configurations");
    struct cli_command *config_running = cli_register_command(cli, show, NULL,
                                                              "config-running", cmd_show_config_running,
                                                              PRIVILEGE_UNPRIVILEGED,
                                                              MODE_ANY, "config-running",
                                                              "show the running configurations");
    struct cli_command *config_startup = cli_register_command(cli, show, NULL,
                                                              "config-startup", cmd_show_config_startup,
                                                              PRIVILEGE_UNPRIVILEGED,
                                                              MODE_ANY, NULL, "show the startup configurations");

    struct cli_command *show_oper = cli_register_command(cli, show, NULL,
                                                         "operational-data", cmd_show_config_startup,
                                                         PRIVILEGE_UNPRIVILEGED,
                                                         MODE_ANY, NULL, "show the operational data");

//    cli_register_optarg(config_running, "format", CLI_CMD_OPTIONAL_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_ANY,
//                        "printed format [json|xml].", NULL, NULL, NULL);

    cli_register_optarg(config_candidate, "format", CLI_CMD_OPTIONAL_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_ANY,
                        "printed format [config-line|json|xml].", NULL, NULL, NULL);


    cli_register_command(cli, NULL, NULL,
                         "discard-changes", cmd_discard_changes, PRIVILEGE_UNPRIVILEGED,
                         MODE_ANY, NULL, "discard all current changes");
    struct cli_command *no_cmd = cli_register_command(cli, NULL, NULL,
                                                      "no", cmd_no, PRIVILEGE_UNPRIVILEGED,
                                                      MODE_ANY, NULL, "delete configs");

    struct cli_ctx_data *ctx_data = (struct cli_ctx_data *) cli_get_context(cli);
    ctx_data->no_cmd = no_cmd;
    ctx_data->show_conf_cand_cmd = config_candidate;
    ctx_data->show_conf_running_cmd = config_running;
    ctx_data->show_conf_startup_cmd = config_startup;
    ctx_data->show_operational_data = show_oper;

    return EXIT_SUCCESS;

}
