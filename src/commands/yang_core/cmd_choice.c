//
// Created by ali on 10/19/23.
//


#include "y_utils.h"
#include "yang_core.h"

int cmd_yang_choice(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (argc == 0) {
        cli_print(cli, "ERROR: please choose one of the available choices for %s, "
                       "use '?' to see available choices", cmd);
        return CLI_MISSING_ARGUMENT;
    } else if (argc > 1) {
        cli_print(cli, "ERROR: please enter one choice for %s", cmd);
        return CLI_MISSING_ARGUMENT;
    }

    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    char xpath[256];

    lysc_path(y_node, LYSC_PATH_DATA, xpath, 256);


    if (y_node != NULL)
        cli_print(cli, "  this command is for module=%s , node=%s, xpath=%s", y_node->module->name, y_node->name,
                  xpath);
    else
        cli_print(cli, "  failed to fine yang module");

    char *mod_str = malloc(sizeof(cmd) + sizeof(argv[0]) + 2);
    sprintf(mod_str, "%s[%s]", (char *) cmd, argv[0]);

    int mode = y_get_next_mode(y_node);

    cli_push_configmode(cli, mode, mod_str);
    return CLI_OK;
}

void replace_space(char *str) {
    while (*str) {
        if (*str == ' ') {
            *str = '_';
        }
        str++;
    }
}

int cmd_yang_case(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    char xpath[256];

    lysc_path(y_node, LYSC_PATH_DATA, xpath, 256);
    cli_print(cli, "  xpath=%s", xpath);
    return CLI_OK;
}

int register_cmd_choice(struct cli_def *cli, struct lysc_node *y_node) {
    char help[100];
    sprintf(help, "configure %s (%s) [choice]", y_node->name, y_node->module->name);
    unsigned int mode;
    const struct lys_module *y_root_module = lysc_owner_module(y_node);
    char *cmd_hash = strdup(y_root_module->name);

    mode = y_get_curr_mode(y_node);

    struct cli_command *choice_cmd = cli_register_command(cli, NULL, y_node, y_node->name,
                                                          cmd_yang_choice, PRIVILEGE_UNPRIVILEGED,
                                                          mode, cmd_hash, help);

    struct lysc_node_choice *y_choice = (struct lysc_node_choice *) y_node;
    struct lysc_node_case *y_case;


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
    LY_LIST_FOR(y_choice->cases, y_case) {
#pragma GCC diagnostic pop
        sprintf(help, "configure %s (%s) [case]", y_case->name, y_case->module->name);
        struct cli_command *case_cmd = cli_register_command(cli, choice_cmd, (void *) y_case, y_case->name,
                                                            cmd_yang_case, PRIVILEGE_UNPRIVILEGED,
                                                            mode, cmd_hash, help);
        const struct lysc_node *case_child_list = lysc_node_child(y_node);
        const struct lysc_node *case_child;
        LY_LIST_FOR(case_child_list, case_child) {
            cli_register_optarg(case_cmd, case_child->name, CLI_CMD_ARGUMENT | CLI_CMD_DO_NOT_RECORD,
                                PRIVILEGE_PRIVILEGED,
                                mode, case_child->dsc, NULL, NULL, NULL);
        }
    }

    return 0;
}