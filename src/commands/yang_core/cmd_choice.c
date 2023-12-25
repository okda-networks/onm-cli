//
// Created by ali on 10/19/23.
//
#include "y_utils.h"
#include "yang_core.h"
#include "data_validators.h"
#include "data_factory.h"


int cmd_yang_choice(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (argc == 0) {
        cli_error(cli, "ERROR: please choose one of the available choices for %s, "
                       "use '?' to see available choices", cmd);
        return CLI_MISSING_ARGUMENT;
    } else if (argc > 1) {
        cli_error(cli, "ERROR: please enter one choice for %s", cmd);
        return CLI_MISSING_ARGUMENT;
    }

    cli_error(cli,"ERROR: invalid choice `%s` , please use `?` for available choices",argv[0]);

    return CLI_ERROR_ARG;
}


int cmd_yang_case(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    struct cli_optarg_pair *optargs;
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    struct lysc_node *y_case_n;


    // case might be leaf or container, for leaf we get the child, for container the child is null and we use y_node
    y_case_n = (struct lysc_node *) lysc_node_child(y_node);
    if (y_case_n == NULL)
        y_case_n = y_node;


    int ret;
    if (argc >= 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, "  <cr>");
            return CLI_OK;
        }
        if (strcmp(argv[0], "delete") == 0) {
            ret = delete_data_node(y_case_n, argv[0],cli);
            if (ret != LY_SUCCESS) {
                cli_print(cli, "Failed to delete the yang data node '%s'\n", y_case_n->name);
                return CLI_ERROR;
            }
            return CLI_OK;
        }
    }



    // if the case node is leaf/leaf-list parse the value, else set the next config mode.
    if (y_case_n->nodetype == LYS_LEAF || y_case_n->nodetype == LYS_LEAFLIST) {
        struct lysc_node *leaf_next;

        LY_LIST_FOR(y_case_n, leaf_next) {
            // add data node
            optargs = cli->found_optargs;
            while (optargs != NULL) {
                if (strcmp(optargs->name, leaf_next->name) == 0) {
                    ret = add_data_node(leaf_next, optargs->value,cli);
                    if (ret != LY_SUCCESS) {
                        cli_print(cli, "Failed to create the yang data node for '%s'\n", y_case_n->name);
                        return CLI_ERROR;
                    }
                    break;
                }

                optargs = optargs->next;
            }
        }


        return CLI_OK;
    }

    // case is container, add data node and move to next mode
    ret = add_data_node(y_case_n, argv[0],cli);
    if (ret != LY_SUCCESS) {
        cli_print(cli, "Failed to create the yang data node for '%s'\n", y_case_n->name);
        return CLI_ERROR;
    }

    // set the next config mode and string.
    char *mod_str = malloc(sizeof("choice[%s]") + strlen(y_node->name));
    sprintf(mod_str, "choice[%s]", (char *) y_node->name);

    int mode;
    mode = y_get_next_mode(y_case_n);

    cli_push_configmode(cli, mode, mod_str);
    free(mod_str);

    return CLI_OK;
}


int register_cmd_choice_core(struct cli_def *cli, struct lysc_node *y_node, struct cli_command *parent_cmd,
                             unsigned int mode) {
    /* ragisterning the choice is the most complex function, change carefully*/
    char help[100];
    const struct lys_module *y_root_module = lysc_owner_module(y_node);

    sprintf(help, "configure %s (%s) [choice]", y_node->name, y_node->module->name);

    char *cmd_hash = (char*)y_root_module->name;
    // this can be called recursively. so we might pass the mode.
    if (mode == -1)
        mode = y_get_curr_mode(y_node);

    struct cli_command *choice_cmd = cli_register_command(cli, parent_cmd, y_node, y_node->name,
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
            // if the case-child is another choice then run recursively
            if (case_child->nodetype == LYS_CHOICE) {
                // should be called with same mode.
                return register_cmd_choice_core(cli, case_child, case_cmd, mode);
            }


            if (case_child->nodetype == LYS_LEAF) {
                struct cli_optarg *o = cli_register_optarg(case_cmd, case_child->name,
                                                           CLI_CMD_ARGUMENT,
                                                           PRIVILEGE_PRIVILEGED,
                                                           mode, case_child->dsc, NULL, yang_data_validator, NULL);
                cli_optarg_addhelp(o, "delete", "delete node from config");
            } else
                register_commands_schema(case_child, cli);
        }
    }

    return 0;
}

int register_cmd_choice(struct cli_def *cli, struct lysc_node *y_node) {
    return register_cmd_choice_core(cli, y_node, NULL, -1);
}