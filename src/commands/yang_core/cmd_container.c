//
// Created by ali on 10/19/23.
//


#include "y_utils.h"
#include "yang_core.h"
#include "data_factory.h"
#include "src/onm_logger.h"


int cmd_yang_container(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    if (y_node->parent != NULL) {
        cli_print(cli, "incomplete command, please use '?' for options cont ");
        return CLI_ERROR;
    }

    int ret;
    int is_delete = 0;


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
    // there is ietf-yang where container and choice has same name.
    // we don't want to register this container to avoid duplication.
    if (y_node->parent != NULL && !strcmp(y_node->parent->name, y_node->name))
        return 1;
    // check if parent is container or choice and is not the root module ,if yes attach the command to the container command.
    struct cli_command *parent_cmd = find_parent_cmd(cli, y_node);

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

    return EXIT_SUCCESS;
}