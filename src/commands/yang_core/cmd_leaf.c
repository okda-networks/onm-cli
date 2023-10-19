//
// Created by ali on 10/19/23.
//

#include "../../utils.h"
#include "yang_core.h"

int cmd_yang_leaf_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (argc == 0) {
        cli_print(cli, "ERROR: please enter value(s) for %s", cmd);
        return CLI_MISSING_ARGUMENT;
    }

    struct lysc_node *ne = (struct lysc_node *) c->cmd_model;
    char xpath[100];

    lysc_path(ne, LYSC_PATH_DATA, xpath, 100);

    if (ne != NULL)
        cli_print(cli, "  xpath=%s\r\n", xpath);
    else
        cli_print(cli, "  failed to find yang module\r\n");
    return CLI_OK;

}

int cmd_yang_leaf(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (argc == 0) {
        cli_print(cli, "ERROR: please enter value for %s", cmd);
        return CLI_MISSING_ARGUMENT;
    } else if (argc > 1) {
        cli_print(cli, "ERROR: please enter one value for %s", cmd);
        return CLI_MISSING_ARGUMENT;
    }

    struct lysc_node *ne = (struct lysc_node *) c->cmd_model;
    char xpath[100];

    lysc_path(ne, LYSC_PATH_DATA, xpath, 100);


    if (ne != NULL)
        cli_print(cli, "  this command is for module=%s , node=%s, xpath=%s[%s=%s]\r\n", ne->module->name, ne->name,
                  xpath, cmd, argv[0]);
    else
        cli_print(cli, "  failed to find yang module\r\n");
    return CLI_OK;

}


int register_cmd_leaf_list(struct cli_def *cli, struct lysc_node *y_node) {
    char help[100];
    sprintf(help, "configure %s (%s) [leaf-list]", y_node->name, y_node->module->name);
    unsigned int mode;
    const struct lys_module *y_module = lysc_owner_module(y_node);

    char *cmd_hash = strdup(y_module->name);;
    if (y_node->parent == NULL)
        mode = MODE_CONFIG;
    else
        mode = str2int_hash(strdup(y_module->name), strdup(y_node->parent->name), NULL);

    struct cli_command *c = cli_register_command(cli, NULL, y_node, y_node->name, cmd_yang_leaf_list,
                                                 PRIVILEGE_PRIVILEGED, mode, cmd_hash, help);
    cli_register_optarg(c, "value(s)", CLI_CMD_ARGUMENT | CLI_CMD_DO_NOT_RECORD, PRIVILEGE_PRIVILEGED, mode,
                        y_node->dsc, NULL, NULL, NULL);

    return 0;
}




int register_cmd_leaf(struct cli_def *cli, struct lysc_node *y_node) {
    char help[100];
    sprintf(help, "configure %s (%s) [leaf]", y_node->name, y_node->module->name);
    unsigned int mode;
    const struct lys_module *y_module = lysc_owner_module(y_node);
    char *cmd_hash = strdup(y_module->name);;
    if (y_node->parent == NULL)
        mode = MODE_CONFIG;
    else
        mode = str2int_hash(strdup(y_module->name), strdup(y_node->parent->name), NULL);


    struct cli_command *c = cli_register_command(cli, NULL, y_node, y_node->name, cmd_yang_leaf,
                                                 PRIVILEGE_PRIVILEGED, mode, cmd_hash, help);
    cli_register_optarg(c, "value", CLI_CMD_ARGUMENT | CLI_CMD_DO_NOT_RECORD, PRIVILEGE_PRIVILEGED, mode,
                        y_node->dsc, NULL, NULL, NULL);

    return 0;
}