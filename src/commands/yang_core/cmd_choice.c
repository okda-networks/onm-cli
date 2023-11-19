//
// Created by ali on 10/19/23.
//
#include "y_utils.h"
#include "yang_core.h"
#include "data_validators.h"
#include "data_factory.h"

// global data tree.
extern struct lyd_node *root_data, *parent_data;


int cmd_yang_choice(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (argc == 0) {
        cli_print(cli, "ERROR: please choose one of the available choices for %s, "
                       "use '?' to see available choices", cmd);
        return CLI_MISSING_ARGUMENT;
    } else if (argc > 1) {
        cli_print(cli, "ERROR: please enter one choice for %s", cmd);
        return CLI_MISSING_ARGUMENT;
    }


    return CLI_OK;
}


int cmd_yang_case(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    struct lysc_node *y_node_child = (struct lysc_node *) lysc_node_child(y_node);

    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, "  <cr>");
            return CLI_OK;
        }
    }

    // add data node
    int ret;
    ret = add_data_node(y_node_child, c, argv[0]);
    if (ret != LY_SUCCESS) {
        cli_print(cli, "Failed to create the yang data node for '%s'\n", y_node_child->name);
        print_ly_err(ly_err_first(y_node_child->module->ctx));
        return CLI_ERROR;
    }

    // if the case node is leaf/leaf-list parse the value, else set the next config mode.
    if (y_node_child->nodetype == LYS_LEAF || y_node_child->nodetype == LYS_LEAFLIST) {
        if (argc == 0) {
            cli_print(cli, "ERROR: please enter value for %s", cmd);
            return CLI_MISSING_ARGUMENT;
        } else if (argc > 1) {
            cli_print(cli, "ERROR: please enter one value for %s", cmd);
            return CLI_MISSING_ARGUMENT;
        }
        return CLI_OK;
    }

    // set the next config mode and string.
    char *mod_str = malloc(sizeof("choice[%s]") + sizeof(y_node->name));
    sprintf(mod_str, "choice[%s]", (char *) y_node->name);

    int mode;
    if (y_node_child != NULL)
        mode = y_get_next_mode(y_node_child);
    else
        mode = y_get_next_mode(y_node);

    cli_push_configmode(cli, mode, mod_str);

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
    struct lysc_node *y_case;

    LY_LIST_FOR((struct lysc_node *) y_choice->cases, y_case) {
        sprintf(help, "configure %s (%s) [case]", y_case->name, y_case->module->name);


        struct cli_command *case_cmd = cli_register_command(cli, choice_cmd, (void *) y_case, y_case->name,
                                                            cmd_yang_case, PRIVILEGE_UNPRIVILEGED,
                                                            mode, cmd_hash, help);
        struct lysc_node *case_child_list = (struct lysc_node *) lysc_node_child(y_case);
        struct lysc_node *case_child;

        LY_LIST_FOR(case_child_list, case_child) {
            if (case_child->nodetype == LYS_LEAF) {
                cli_register_optarg(case_cmd, case_child->name, CLI_CMD_ARGUMENT | CLI_CMD_DO_NOT_RECORD,
                                    PRIVILEGE_PRIVILEGED,
                                    mode, case_child->dsc, NULL, yang_data_validator, NULL);
            } else {
                register_commands_schema(case_child, cli);
            }


        }
    }

    return 0;
}