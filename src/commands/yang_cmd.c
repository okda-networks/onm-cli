//
// Created by ali on 10/7/23.
//

#include "yang_cmd.h"
#include "../utils.h"

struct lysc_node *get_root_module_name(struct lysc_node *node) {
    struct lysc_node *root = node;

    // Traverse up the module hierarchy until we reach the root node
    while (root->parent != NULL) {
        root = root->parent;
    }

    return root;
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
                  xpath,cmd,argv[0]);
    else
        cli_print(cli, "  failed to find yang module\r\n");
    return CLI_OK;

}

int cmd_yang_container(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, "  %s", c->help);
            return CLI_INCOMPLETE_COMMAND;
        }
        cli_print(cli, "  unknown args\n");
        return CLI_ERROR_ARG;
    }
    struct lysc_node *ne = (struct lysc_node *) c->cmd_model;
    const struct lys_module *y_module = lysc_owner_module(ne);


    int mode = str2int_hash(strdup(y_module->name), strdup(ne->name), NULL);
    cli_push_configmode(cli, mode, (char *) cmd);
    return CLI_OK;
}

int cmd_yang_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (argc == 0) {
        cli_print(cli, "ERROR: please enter %s of %s entry", c->optargs->name, cmd);
        return CLI_MISSING_ARGUMENT;
    } else if (argc > 1) {
        cli_print(cli, "ERROR: please enter one Key(%s) for the list entry of %s", c->optargs->name, cmd);
        return CLI_MISSING_ARGUMENT;
    }

    struct lysc_node *ne = (struct lysc_node *) c->cmd_model;
    char xpath[100];

    lysc_path(ne, LYSC_PATH_DATA, xpath, 100);


    if (ne != NULL)
        cli_print(cli, "  this command is for module=%s , node=%s, xpath=%s", ne->module->name, ne->name, xpath);
    else
        cli_print(cli, "  failed to fine yang module");

    char *mod_str = malloc(sizeof(cmd) + sizeof(argv[0]) + 2);
    sprintf(mod_str, "%s[%s]", (char *) cmd, argv[0]);
    const struct lys_module *y_module = lysc_owner_module(ne);

    int mode = str2int_hash(strdup(y_module->name), strdup(ne->name), NULL);

    cli_push_configmode(cli, mode, mod_str);
    return CLI_OK;
}


