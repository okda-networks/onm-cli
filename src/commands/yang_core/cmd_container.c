//
// Created by ali on 10/19/23.
//


#include "y_utils.h"
#include "yang_core.h"
#include "data_factory.h"
#include "src/onm_logger.h"


int cmd_yang_container(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    int ret;
    int is_delete=0;
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;

    if (argc >= 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, " <cr>     configure container: %s", y_node->dsc);
            return CLI_INCOMPLETE_COMMAND;
        }
        cli_print(cli, "ERROR: unknown argument(s)");
        return CLI_ERROR_ARG;
    }

    struct cli_optarg_pair *optargs = cli->found_optargs;

    while (optargs != NULL){
        if (strcmp(optargs->name,"delete")==0)
            is_delete =1;
        optargs = optargs->next;
    }

    if (is_delete) {
        ret = delete_data_node(y_node, NULL,cli);
        if (ret != LY_SUCCESS) {
            LOG_ERROR( "Failed to delete the data tree");
            cli_error(cli, "failed to execute command, error with deleting the data node.");
            return CLI_ERROR;
        } else
            return CLI_OK;
    }

    // this is add operation.
    ret = add_data_node(y_node, NULL,cli);
    if (ret != LY_SUCCESS) {
        LOG_ERROR( "Failed to create the data tree");
        cli_error(cli, "failed to execute command, error with adding the data node.");
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
    char *cmd_hash = (char*)y_root_module->name;

    mode = y_get_curr_mode(y_node);


    struct cli_command *c = cli_register_command(cli, NULL, y_node, y_node->name,
                                                 cmd_yang_container, PRIVILEGE_UNPRIVILEGED,
                                                 mode, cmd_hash, help);
    cli_register_optarg(c, "delete", CLI_CMD_OPTIONAL_FLAG,
                        PRIVILEGE_PRIVILEGED, mode,
                        "delete container", NULL, NULL, NULL);
    return 0;
}