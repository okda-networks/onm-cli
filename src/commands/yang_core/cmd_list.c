//
// Created by ali on 10/19/23.
//
#include "src/utils.h"
#include "y_utils.h"
#include "yang_core.h"
#include "data_validators.h"
#include "data_factory.h"
#include "src/onm_logger.h"
#include "data_cmd_compl.h"

int cmd_print_list_order(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    int curr_indx = 10;
    struct lyd_node *next = NULL, *entry_child = NULL;
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    struct lyd_node *list_entries = get_local_list_nodes(y_node);
    char line[265] = {'\0'};
    LY_LIST_FOR(list_entries, next) {
        struct lyd_node *entry_children = lyd_child(next);
        strcat(line, next->schema->name);
        LY_LIST_FOR(entry_children, entry_child) {
            if (lysc_is_key(entry_child->schema)) {
                strcat(line, " ");
                strcat(line, lyd_get_value(entry_child));
            }
        }
        cli_print(cli, "%s index [%d]", line, curr_indx);
        memset(line, '\0', 265);
        curr_indx += 10;
    }
    return EXIT_SUCCESS;

}

int cmd_yang_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, "  <cr>");
            cli_print(cli, "  delete");
            return CLI_OK;
        } else {
            cli_error(cli, "ERROR: unknown argument '%s'", argv[0]);
            return CLI_ERROR_ARG;
        }
    }


    struct cli_optarg_pair *optargs = cli->found_optargs;
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    int index = 0; // no index

    int is_delete = cli_get_optarg_value(cli, "delete", NULL) ? 1 : 0;
    char *idx_char = cli_get_optarg_value(cli, "index", NULL);
    if (idx_char != NULL) {
        char *idx_endptr;
        index = (int) strtol(idx_char, &idx_endptr, 10);

        // Check for conversion errors
        if ((*idx_endptr != '\0' && *idx_endptr != '\n') || (index == 0)) {
            cli_error(cli, "ERROR: <index> must be numeric greater than 0, entered value=%s", optargs->value);
            return CLI_ERROR;
        }
    }

    int has_none_key_node = 0;
    const struct lysc_node *child_list = lysc_node_child(y_node);
    const struct lysc_node *child;
    // if list has none key nodes we will not update data parent_node and we will not go to new mode.
    LY_LIST_FOR(child_list, child) {
        if (!lysc_is_key(child)) {
            has_none_key_node = 1;
            break;
        }

    }

    int ret;
    optargs = cli->found_optargs;
    create_argv_from_optpair(optargs, &argv, &argc);
    if (is_delete) {
        ret = delete_data_node_list(y_node, argv, argc, cli);
        if (ret != LY_SUCCESS) {
            LOG_ERROR("Failed to delete the data tree");
            cli_print(cli, "failed to execute command, error with adding the data node.");
            free_argv(argv, argc);
            return CLI_ERROR;
        }
        return CLI_OK;
    } else {
        ret = add_data_node_list(y_node, argv, argc, index, cli, has_none_key_node);
    }


    if (ret != LY_SUCCESS) {
        LOG_ERROR("Failed to create/delete the data tree");
        cli_print(cli, "failed to execute command, error with adding the data node. list");
        free_argv(argv, argc);
        return CLI_ERROR;
    }
    if (!has_none_key_node) {
        free_argv(argv, argc);
        return CLI_OK;
    }


    char *mod_str;
    ssize_t mod_str_len = strlen(cmd) + 3;
    for (int i = 0; i < argc; i++) {
        mod_str_len += strlen(argv[i]) + 2;
    }
    mod_str = malloc(mod_str_len);
    memset(mod_str, 0, mod_str_len);
    strcat(mod_str, (char *) cmd);
    for (int i = 0; i < argc; i++) {
        strcat(mod_str, "[");
        strcat(mod_str, argv[i]);
        strcat(mod_str, "]");
    }

    int mode = y_get_next_mode(y_node);
    cli_push_configmode(cli, mode, mod_str);

    free(mod_str);
    free_argv(argv, argc);
    return CLI_OK;
}

int cmd_yang_no_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, "  <cr>");
            return CLI_OK;
        } else {
            cli_error(cli, "ERROR: unknown argument '%s'", argv[0]);
            return CLI_ERROR_ARG;
        }
    }
    int ret;
    struct cli_optarg_pair *optargs = cli->found_optargs;
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;


    create_argv_from_optpair(optargs, &argv, &argc);

    ret = delete_data_node_list(y_node, argv, argc, cli);
    if (ret != LY_SUCCESS) {
        LOG_ERROR("Failed to delete the data tree");
        cli_print(cli, "failed to execute command, error with adding the data node.");
        free_argv(argv, argc);
        return CLI_ERROR;
    }
    free_argv(argv, argc);
    return CLI_OK;
}

enum {
    RUNNING_DS,
    STARTUP_DS,
    CANDIDATE_DS,
    OPERATIONAL_DS
};

int core_yand_show_config(struct cli_def *cli, struct cli_command *c, int datastore) {
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    char xpath[1028] = {0};
    lysc_path(y_node, LYSC_PATH_DATA, xpath, 1028);

    const struct lysc_node *child_list = lysc_node_child(y_node);
    const struct lysc_node *child;

    LY_LIST_FOR(child_list, child) {
        if (lysc_is_key(child)) {
            strcat(xpath, "[");
            strcat(xpath, child->name);
            strcat(xpath, "='");
            strcat(xpath, cli_get_optarg_value(cli, child->name, NULL));
            strcat(xpath, "']");
        }
    }
    int is_diff = cli_get_optarg_value(cli, "diff", NULL) ? 1 : 0;
    struct lyd_node *d_node = NULL;

    if (is_diff) {
        if (datastore != CANDIDATE_DS) {
            cli_print(cli, "config diff is support for config-candidate only.");
            return CLI_ERROR;
        }
        struct lyd_node *candidate_node = get_local_node_data(xpath);
        if (candidate_node == NULL) {
            cli_print(cli, " no config diff between candidate and running for node '%s'", xpath);
            return CLI_OK;
        }
        struct lyd_node *running_node = get_sysrepo_running_node(xpath);
        lyd_diff_tree(running_node, candidate_node, 0, &d_node);
        if (d_node)
            config_print(cli, d_node);
        else {
            cli_print(cli, " no config diff between candidate and running for node '%s'", xpath);
            return CLI_OK;
        }

    } else {
        switch (datastore) {
            case CANDIDATE_DS:
                d_node = get_local_node_data(xpath);
                break;
            case RUNNING_DS:
                d_node = get_sysrepo_running_node(xpath);
                break;
            case STARTUP_DS:
                d_node = get_sysrepo_startup_node(xpath);
                break;
            case OPERATIONAL_DS:
                d_node = get_sysrepo_operational_node(xpath);
                break;
        }
    }


    if (d_node)
        config_print(cli, d_node);
    else
        cli_print(cli, " no data found for node '%s'", xpath);

    return CLI_OK;
}

int cmd_yang_show_candidate_config_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[],
                                        int argc) {
    if (argc >= 1) {
        cli_print(cli, "ERROR: unknown argument(s)");
        return CLI_ERROR_ARG;
    }
    return core_yand_show_config(cli, c, CANDIDATE_DS);

}

int cmd_yang_show_running_config_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[],
                                      int argc) {
    if (argc >= 1) {
        cli_print(cli, "ERROR: unknown argument(s)");
        return CLI_ERROR_ARG;
    }
    return core_yand_show_config(cli, c, RUNNING_DS);

}

int cmd_yang_show_startup_config_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[],
                                      int argc) {
    if (argc >= 1) {
        cli_print(cli, "ERROR: unknown argument(s)");
        return CLI_ERROR_ARG;
    }
    return core_yand_show_config(cli, c, STARTUP_DS);
}

int cmd_yang_show_operational_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[],
                                   int argc) {
    if (argc >= 1) {
        cli_print(cli, "ERROR: unknown argument(s)");
        return CLI_ERROR_ARG;
    }
    return core_yand_show_config(cli, c, OPERATIONAL_DS);

}

int register_cmd_list(struct cli_def *cli, struct lysc_node *y_node) {
    const struct lys_module *y_root_module = lysc_owner_module(y_node);
    char *cmd_hash = (char *) y_root_module->name;

    if (has_oper_children(y_node)) {
        char show_oper_help[100] = {0};
        char *oper_optarg_help;
        sprintf(show_oper_help, "show operational data for %s (%s) [list]", y_node->name, y_node->module->name);
        struct cli_command *show_oper_c, *show_oper_c_parent;

        show_oper_c_parent = find_parent_show_oper_cmd(cli, y_node);
        if (show_oper_c_parent != NULL) {
            show_oper_c = cli_register_command(cli, show_oper_c_parent, y_node,
                                               y_node->name,
                                               cmd_yang_show_operational_list,
                                               PRIVILEGE_PRIVILEGED,
                                               MODE_ANY, cmd_hash, show_oper_help);
            const struct lysc_node *child_list = lysc_node_child(y_node);
            const struct lysc_node *child;
            LY_LIST_FOR(child_list, child) {
                if (lysc_is_key(child)) {
                    if (lysc_is_key(child)) {
                        if (child->dsc != NULL)
                            oper_optarg_help = strdup(child->dsc);
                        else {
                            oper_optarg_help = malloc(strlen(child->name) + strlen("configure ") + 2);
                            sprintf((char *) oper_optarg_help, "configure %s", strdup(child->name));
                        }
                        struct cli_optarg *show_oper_o = cli_register_optarg(show_oper_c, child->name, CLI_CMD_ARGUMENT,
                                                                             PRIVILEGE_PRIVILEGED,
                                                                             MODE_ANY, oper_optarg_help,
                                                                             optagr_get_compl_running,
                                                                             yang_data_validator, NULL);
                        show_oper_o->opt_model = (void *) child;
                        free(oper_optarg_help);

                    }
                }
            }
        }

    }
    if (y_node->flags & LYS_CONFIG_R)
        return CLI_OK;

    unsigned int mode;


    char help[100], no_help[100], show_help[100];
    sprintf(help, "configure %s (%s) [list]", y_node->name, y_node->module->name);
    sprintf(no_help, "delete %s (%s) [list]", y_node->name, y_node->module->name);
    sprintf(show_help, "show %s configurations (%s)", y_node->name, y_node->module->name);

    struct cli_command *parent_cmd = find_parent_cmd(cli, y_node);
    struct cli_command *parent_cmd_no = find_parent_no_cmd(cli, y_node);
    struct cli_command *parent_cmd_show_conf_cand = find_parent_show_candidate_cmd(cli, y_node);
    struct cli_command *parent_cmd_show_conf_run = find_parent_show_running_cmd(cli, y_node);
    struct cli_command *parent_cmd_show_conf_start = find_parent_show_startup_cmd(cli, y_node);

    struct cli_command *show_cmd_cand = NULL, *show_cmd_run = NULL, *show_cmd_start = NULL;
    if (parent_cmd_show_conf_cand != NULL) {
        show_cmd_cand = cli_register_command(cli, parent_cmd_show_conf_cand, y_node, y_node->name,
                                             cmd_yang_show_candidate_config_list, PRIVILEGE_PRIVILEGED,
                                             MODE_ANY, cmd_hash, show_help);
    }
    if (parent_cmd_show_conf_run != NULL) {
        show_cmd_run = cli_register_command(cli, parent_cmd_show_conf_run, y_node, y_node->name,
                                            cmd_yang_show_running_config_list, PRIVILEGE_PRIVILEGED,
                                            MODE_ANY, cmd_hash, show_help);
    }
    if (parent_cmd_show_conf_start != NULL) {
        show_cmd_start = cli_register_command(cli, parent_cmd_show_conf_start, y_node, y_node->name,
                                              cmd_yang_show_startup_config_list, PRIVILEGE_PRIVILEGED,
                                              MODE_ANY, cmd_hash, show_help);
    }


    if (parent_cmd == NULL)
        mode = y_get_curr_mode(y_node);
    else
        mode = parent_cmd->mode;

    if (parent_cmd_no == NULL)
        parent_cmd_no = ((struct cli_ctx_data *) cli_get_context(cli))->no_cmd;


    struct cli_command *c = cli_register_command(cli, parent_cmd, y_node, y_node->name, cmd_yang_list,
                                                 PRIVILEGE_PRIVILEGED, mode, cmd_hash, help);

    struct cli_command *no_c = cli_register_command(cli, parent_cmd_no, y_node, y_node->name,
                                                    cmd_yang_no_list, PRIVILEGE_PRIVILEGED,
                                                    mode, cmd_hash, no_help);

    const struct lysc_node *child_list = lysc_node_child(y_node);
    const struct lysc_node *child;
    struct cli_optarg *o, *no_o, *show_o;
//    char *list_cmd_help = create_list_cmd_help(y_node, lysc_is_userordered(y_node));

    LY_LIST_FOR(child_list, child) {
        if (lysc_is_key(child)) {
            const char *optarg_help;
            LY_DATA_TYPE type = ((struct lysc_node_leaf *) child)->type->basetype;
            if (type == LY_TYPE_IDENT)
                optarg_help = creat_help_for_identity_type((struct lysc_node *) child);
            else {
                if (child->dsc != NULL)
                    optarg_help = strdup(child->dsc);
                else {
                    optarg_help = malloc(strlen(child->name) + strlen("configure ") + 2);
                    sprintf((char *) optarg_help, "configure %s", strdup(child->name));
                }
            }

            o = cli_register_optarg(c, child->name, CLI_CMD_ARGUMENT, PRIVILEGE_PRIVILEGED,
                                    mode, optarg_help, optagr_get_compl_candidate, yang_data_validator, NULL);
            no_o = cli_register_optarg(no_c, child->name, CLI_CMD_ARGUMENT, PRIVILEGE_PRIVILEGED,
                                       mode, optarg_help, optagr_get_compl_candidate, yang_data_validator, NULL);
            o->opt_model = (void *) child; // for get_completion
            no_o->opt_model = (void *) child; // for get_completion

            if (parent_cmd_show_conf_cand != NULL) {
                show_o = cli_register_optarg(show_cmd_cand, child->name, CLI_CMD_ARGUMENT, PRIVILEGE_PRIVILEGED,
                                             MODE_ANY, optarg_help, optagr_get_compl_candidate, yang_data_validator,
                                             NULL);

                show_o->opt_model = (void *) child;// for get_completion
            }
            if (parent_cmd_show_conf_start != NULL) {
                show_o = cli_register_optarg(show_cmd_start, child->name, CLI_CMD_ARGUMENT, PRIVILEGE_PRIVILEGED,
                                             MODE_ANY, optarg_help, optagr_get_compl_startup, yang_data_validator,
                                             NULL);
                show_o->opt_model = (void *) child;// for get_completion
            }
            if (parent_cmd_show_conf_run != NULL) {
                show_o = cli_register_optarg(show_cmd_run, child->name, CLI_CMD_ARGUMENT, PRIVILEGE_PRIVILEGED,
                                             MODE_ANY, optarg_help, optagr_get_compl_running, yang_data_validator,
                                             NULL);
                show_o->opt_model = (void *) child;// for get_completion
            }
            free((char *) optarg_help);
        }
    }

    // add diff optargs for parent_cmd_show_conf_cand
    if (parent_cmd_show_conf_cand != NULL) {
        cli_register_optarg(show_cmd_cand, "diff", CLI_CMD_OPTIONAL_FLAG, PRIVILEGE_PRIVILEGED,
                            MODE_ANY, strdup("show difference"), NULL, yang_data_validator, NULL);
    }
    if (lysc_is_userordered(y_node)) {
        cli_register_optarg(c, "index", CLI_CMD_OPTIONAL_ARGUMENT, PRIVILEGE_PRIVILEGED,
                            mode, "to delete the list entry", NULL, NULL, NULL);

        // add print order command

        struct cli_command *print_order = cli_register_command(cli, NULL, NULL, "print-order", NULL,
                                                               PRIVILEGE_PRIVILEGED, mode, cmd_hash,
                                                               "print ordered entries of the list");

        cli_register_command(cli, print_order, y_node, y_node->name, cmd_print_list_order,
                             PRIVILEGE_PRIVILEGED, mode, cmd_hash,
                             "print ordered entries of the list");

    }


    return 0;
}
