//
// Created by ali on 10/19/23.
//

#include "y_utils.h"
#include "yang_core.h"

int cmd_yang_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (argc == 0) {
        cli_print(cli, "ERROR: please enter %s of %s entry", c->optargs->name, cmd);
        return CLI_MISSING_ARGUMENT;
    } else if (argc > 1) {
        cli_print(cli, "ERROR: please enter one Key(%s) for the list entry of %s", c->optargs->name, cmd);
        return CLI_MISSING_ARGUMENT;
    }

    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    char xpath[256];

    lysc_path(y_node, LYSC_PATH_DATA, xpath, 256);


    if (y_node != NULL)
        cli_print(cli, "  this command is for module=%s , node=%s, xpath=%s", y_node->module->name, y_node->name, xpath);
    else
        cli_print(cli, "  failed to fine yang module");

    char *mod_str = malloc(sizeof(cmd) + sizeof(argv[0]) + 2);
    sprintf(mod_str, "%s[%s]", (char *) cmd, argv[0]);

    int mode = y_get_next_mode(y_node);

    cli_push_configmode(cli, mode, mod_str);
    return CLI_OK;
}

struct lysc_node * get_root_module_name(struct lysc_node *node) {
    struct lysc_node *root = node;

    // Traverse up the module hierarchy until we reach the root node
    while (root->parent != NULL) {
        root = root->parent;
    }

    return root;
}

int register_cmd_list(struct cli_def *cli, struct lysc_node *y_node) {
    char help[100];
    sprintf(help, "configure %s (%s) [list]", y_node->name, y_node->module->name);
    unsigned int mode;
    const struct lys_module *y_root_module = lysc_owner_module(y_node);

    char *cmd_hash = strdup(y_root_module->name);;
    mode = y_get_curr_mode(y_node);

    struct cli_command *c = cli_register_command(cli, NULL, y_node, y_node->name, cmd_yang_list,
                                                 PRIVILEGE_PRIVILEGED, mode, cmd_hash, help);

    const struct lysc_node *child_list = lysc_node_child(y_node);
    const struct lysc_node *child;
    LYSC_TREE_DFS_BEGIN(child_list, child) {
        if (child->flags & LYS_KEY) {
            cli_register_optarg(c, child->name, CLI_CMD_ARGUMENT | CLI_CMD_DO_NOT_RECORD, PRIVILEGE_PRIVILEGED, mode,
                                child->dsc, NULL, NULL, NULL);
            break;
        }
        LYSC_TREE_DFS_END(child_list->next, child);
    }
    return 0;
}
