//
// Created by ali on 10/19/23.
//


#include "y_utils.h"
#include "yang_core.h"
#include "data_factory.h"
#include "src/onm_logger.h"


int cmd_yang_container(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    // execut parent containers commands to build parent_data tree.

//    if (c == NULL)
//        return CLI_OK;
//    if (c->parent != NULL)
//        c->parent->callback(cli, c->parent, c->parent->command, NULL, -1);

    int ret;
    int is_delete = 0;
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;

    if (argc >= 1) {
        cli_print(cli, "ERROR: unknown argument(s)");
        return CLI_ERROR_ARG;
    }

    struct cli_optarg_pair *optargs = cli->found_optargs;

    while (optargs != NULL) {
        if (strcmp(optargs->name, "delete") == 0)
            is_delete = 1;
        optargs = optargs->next;
    }

    if (is_delete) {
        ret = delete_data_node(y_node, NULL, cli);
        if (ret != LY_SUCCESS) {
            LOG_ERROR("Failed to delete the data tree");
            cli_error(cli, "failed to execute command, error with deleting the data node.");
            return CLI_ERROR;
        } else
            return CLI_OK;
    }

    // this is add operation.
    ret = add_data_node(y_node, NULL, cli);
    if (ret != LY_SUCCESS) {
        LOG_ERROR("Failed to create the data tree");
        cli_error(cli, "failed to execute command, error with adding the data node.");
        return CLI_ERROR;
    }

    // check if this is the root container which is the only container that changes config mode.
    if (y_node->parent == NULL) {
        int mode = y_get_next_mode(y_node);
        cli_push_configmode(cli, mode, (char *) cmd);
    }


    return CLI_OK;
}

int register_cmd_container(struct cli_def *cli, struct lysc_node *y_node) {
    char help[100];
    sprintf(help, "configure %s (%s) [contain]", y_node->name, y_node->module->name);
    unsigned int mode;
    const struct lys_module *y_root_module = lysc_owner_module(y_node);
    char *cmd_hash = (char *) y_root_module->name;

    struct cli_command *parent_cmd = NULL;
    // check if parent is container or choice and is not the root module ,if yes attach the command to the container command.
    if (y_node->parent != NULL && y_node->parent->parent != NULL
        && (y_node->parent->nodetype == LYS_CONTAINER || y_node->parent->nodetype == LYS_CHOICE || y_node->parent->nodetype == LYS_CASE)) {
        if (y_node->parent->nodetype == LYS_CASE)
            parent_cmd = get_cli_yang_command(cli, &y_node->parent->parent);
        else
            parent_cmd = get_cli_yang_command(cli, &y_node->parent);
    }


    if (parent_cmd == NULL)
        mode = y_get_curr_mode(y_node);
    else
        mode = parent_cmd->mode;

    struct cli_command *c = cli_register_command(cli, parent_cmd, y_node, y_node->name,
                                                 cmd_yang_container, PRIVILEGE_PRIVILEGED,
                                                 mode, cmd_hash, help);
    cli_register_optarg(c, "delete", CLI_CMD_OPTIONAL_FLAG,
                        PRIVILEGE_PRIVILEGED, mode,
                        "delete container", NULL, NULL, NULL);

    return 0;
}