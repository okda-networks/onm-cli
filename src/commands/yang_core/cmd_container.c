//
// Created by ali on 10/19/23.
//


#include "y_utils.h"
#include "yang_core.h"
#include "data_factory.h"


int cmd_yang_container(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    int ret;

    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, "  %s", y_node->dsc);
            return CLI_INCOMPLETE_COMMAND;
        } else if (strcmp(argv[0], "delete") == 0) {
            ret = delete_data_node(y_node,NULL);
            if (ret != LY_SUCCESS) {
                fprintf(stderr, "Failed to delete the data tree\n");
                cli_print(cli, "failed to execute command, error with deleting the data node.");
                return CLI_ERROR;
            } else
                return CLI_OK;
        }
        cli_print(cli, "  unknown argument\n");
        return CLI_ERROR_ARG;
    }


    ret = add_data_node(y_node, argv[0]);


    if (ret != LY_SUCCESS) {
        fprintf(stderr, "Failed to create the data tree\n");
        cli_print(cli, "failed to execute command, error with adding the data node.");
        return CLI_ERROR;
    }


    int mode = y_get_next_mode(y_node);
    cli_push_configmode(cli, mode, (char *) cmd);
    return CLI_OK;
}

int register_cmd_container(struct cli_def *cli, struct lysc_node *y_node) {
    char help[100];
    sprintf(help, "configure %s (%s) [contain]", y_node->name, y_node->module->name);
    unsigned int mode;
    const struct lys_module *y_root_module = lysc_owner_module(y_node);
    char *cmd_hash = strdup(y_root_module->name);

    mode = y_get_curr_mode(y_node);


    cli_register_command(cli, NULL, y_node, y_node->name,
                         cmd_yang_container, PRIVILEGE_UNPRIVILEGED,
                         mode, cmd_hash, help);
    return 0;
}