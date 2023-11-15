//
// Created by ali on 10/19/23.
//


#include "y_utils.h"
#include "yang_core.h"

// global data tree.
extern struct lyd_node *root_data, *parent_data;

int cmd_yang_container(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, "  %s", y_node->dsc);
            return CLI_INCOMPLETE_COMMAND;
        }
        cli_print(cli, "  unknown argument\n");
        return CLI_ERROR_ARG;
    }

    // for container we need to confirm if the parent is null then this is the first child of the root
    // if it's not null then add the container to the current parent.
    char xpath[256];
    int ret;
    if (root_data == NULL){
        lysc_path(y_node, LYSC_PATH_DATA, xpath, 256);
        ret = lyd_new_path(NULL, y_node->module->ctx, xpath, NULL, 0, &root_data);
    }

    else{
        snprintf(xpath, 256, "%s:%s", y_node->module->name, y_node->name);
        ret = lyd_new_path(parent_data, y_node->module->ctx, xpath,
                            NULL,LYD_NEW_PATH_UPDATE,&parent_data);
    }



    if (ret != LY_SUCCESS) {
        fprintf(stderr, "Failed to create the data tree\n");
        print_ly_err(ly_err_first(y_node->module->ctx));
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