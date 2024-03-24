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

#include "y_utils.h"
#include "yang_core.h"
#include "data_validators.h"
#include "data_factory.h"
#include "data_cmd_compl.h"


int cmd_yang_leaf_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (argc == 0) {
        cli_print(cli, "ERROR: please enter value(s) for %s", cmd);
        return CLI_MISSING_ARGUMENT;
    }
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    int ret;

    // libcli does not support validating mutiple values for same optarg, this is a WA to validate all values.
    for (int i = 0; i < argc; i++) {
        if (yang_data_validator(cli, cmd, argv[i], c->cmd_model) != CLI_OK) return CLI_ERROR_ARG;
        ret = add_data_node(y_node, argv[i], cli);
        if (ret != LY_SUCCESS) {
            cli_print(cli, "Failed to create the yang data node for '%s'\n", y_node->name);
            return CLI_ERROR;
        }
    }

    return CLI_OK;

}

int cmd_yang_no_leaf_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (argc == 0) {
        cli_print(cli, "ERROR: please enter value(s) for %s", cmd);
        return CLI_MISSING_ARGUMENT;
    }
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    int ret;

    // check if it's delete action `<leaf-list> <value> delete`

    ret = delete_data_node(y_node, argv[0], cli);
    if (ret != LY_SUCCESS) {
        cli_print(cli, "Failed to delete the yang data node for '%s'\n", y_node->name);
        return CLI_ERROR;
    }
    return CLI_OK;

}


int cmd_yang_leaf(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    int ret;
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    char *value = cli_get_optarg_value(cli, "value", NULL);
    ret = add_data_node(y_node, value, cli);
    if (ret != LY_SUCCESS) {
        cli_print(cli, "Failed to create the yang data node for '%s'\n", y_node->name);
        return CLI_ERROR;
    }
    return CLI_OK;

}

int cmd_yang_no_leaf(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    int ret;
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    ret = delete_data_node(y_node, NULL, cli);
    if (ret != LY_SUCCESS) {
        cli_print(cli, "Failed to delete the yang data node for '%s'\n", y_node->name);
        return CLI_ERROR;
    }
    return CLI_OK;
}

int register_cmd_leaf_list(struct cli_def *cli, struct lysc_node *y_node) {
    if (y_node->flags & LYS_CONFIG_R)
        return 1;

    char help[100], no_help[100];
    sprintf(help, "configure %s (%s) [leaf-list]", y_node->name, y_node->module->name);
    sprintf(no_help, "delete %s (%s) [leaf-list]", y_node->name, y_node->module->name);
    unsigned int mode;
    char *optarg_help;
    const struct lys_module *y_module = lysc_owner_module(y_node);


    char *cmd_hash = (char *) y_module->name;

    struct cli_command *parent_cmd = find_parent_cmd(cli, y_node);
    struct cli_command *parent_cmd_no = find_parent_no_cmd(cli, y_node);
    if (parent_cmd_no == NULL)
        parent_cmd_no = ((struct cli_ctx_data *) cli_get_context(cli))->no_cmd;


    if (parent_cmd == NULL)
        mode = y_get_curr_mode(y_node);
    else
        mode = parent_cmd->mode;


    if (y_node->dsc != NULL)
        optarg_help = strdup(y_node->dsc);
    else {
        optarg_help = malloc(strlen(y_node->name) + strlen("configure ") + 2);
        sprintf((char *) optarg_help, "configure %s", strdup(y_node->name));
    }


    struct cli_command *c = cli_register_command(cli, parent_cmd, y_node, y_node->name, cmd_yang_leaf_list,
                                                 PRIVILEGE_PRIVILEGED, mode, cmd_hash, help);

    struct cli_command *no_c = cli_register_command(cli, parent_cmd_no, y_node, y_node->name,
                                                    cmd_yang_no_leaf_list, PRIVILEGE_PRIVILEGED,
                                                    mode, cmd_hash, no_help);

    cli_register_optarg(c, "value(s)",
                        CLI_CMD_ARGUMENT | CLI_CMD_DO_NOT_RECORD | CLI_CMD_OPTION_MULTIPLE,
                        PRIVILEGE_PRIVILEGED, mode,
                        optarg_help, NULL, NULL, NULL);

    cli_register_optarg(no_c, "value(s)",
                        CLI_CMD_ARGUMENT | CLI_CMD_DO_NOT_RECORD | CLI_CMD_OPTION_MULTIPLE,
                        PRIVILEGE_PRIVILEGED, mode,
                        optarg_help, NULL, NULL, NULL);


    free(optarg_help);

    return 0;
}


int register_cmd_leaf(struct cli_def *cli, struct lysc_node *y_node) {
    if (y_node->flags & LYS_CONFIG_R)
        return 1;

    char help[100], no_help[100];
    sprintf(help, "configure %s (%s) [leaf]", y_node->name, y_node->module->name);
    sprintf(no_help, "delete %s (%s) [leaf]", y_node->name, y_node->module->name);

    unsigned int mode;
    struct cli_comphelp *comphelp = NULL;
    const struct lys_module *y_module = lysc_owner_module(y_node);
    char *cmd_hash = (char *) y_module->name;

    struct cli_command *parent_cmd = find_parent_cmd(cli, y_node);
    struct cli_command *parent_cmd_no = find_parent_no_cmd(cli, y_node);
    if (parent_cmd_no == NULL)
        parent_cmd_no = ((struct cli_ctx_data *) cli_get_context(cli))->no_cmd;


    if (parent_cmd == NULL)
        mode = y_get_curr_mode(y_node);
    else
        mode = parent_cmd->mode;


    struct cli_command *c = cli_register_command(cli, parent_cmd, y_node, y_node->name, cmd_yang_leaf,
                                                 PRIVILEGE_PRIVILEGED, mode, cmd_hash, help);
    cli_register_command(cli, parent_cmd_no, y_node, y_node->name,
                         cmd_yang_no_leaf, PRIVILEGE_PRIVILEGED,
                         mode, cmd_hash, no_help);

    const char *optarg_help;
    LY_DATA_TYPE type = ((struct lysc_node_leaf *) y_node)->type->basetype;
    if (type == LY_TYPE_IDENT)
        optarg_help = creat_help_for_identity_type(y_node);
    else {
        if (y_node->dsc != NULL)
            optarg_help = strdup(y_node->dsc);
        else {
            optarg_help = malloc(strlen(y_node->name) + strlen("configure ") + 2);
            sprintf((char *) optarg_help, "configure %s", strdup(y_node->name));
        }
    }

    if (type == LY_TYPE_ENUM) {
        struct lysc_type_enum *y_enum_type = (struct lysc_type_enum *) ((struct lysc_node_leaf *) y_node)->type;
        LY_ARRAY_COUNT_TYPE i;
        LY_ARRAY_FOR(y_enum_type->enums, i) {
            cli_add_comphelp_entry(comphelp, y_enum_type->enums[i].name);
        }
    }

    struct cli_optarg *o = cli_register_optarg(c, "value", CLI_CMD_ARGUMENT,
                                               PRIVILEGE_PRIVILEGED, mode,
                                               optarg_help, optagr_get_compl_candidate_running, yang_data_validator, NULL);
    o->opt_model = (void *) y_node;
    free((char *) optarg_help);

    return 0;
}