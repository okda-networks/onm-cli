//
// Created by ali on 10/19/23.
//
#include "y_utils.h"
#include "yang_core.h"
#include "data_validators.h"
#include "data_factory.h"
#include "data_cmd_compl.h"

int cmd_yang_choice(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (argc == 0) {
        cli_error(cli, "ERROR: please choose one of the available choices for %s, "
                       "use '?' to see available choices", cmd);
        return CLI_MISSING_ARGUMENT;
    } else if (argc > 1) {
        cli_error(cli, "ERROR: please enter one choice for %s", cmd);
        return CLI_MISSING_ARGUMENT;
    }

    cli_error(cli, "ERROR: invalid choice `%s` , please use `?` for available choices", argv[0]);

    return CLI_ERROR_ARG;
}

int cmd_yang_no_choice(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    return cmd_yang_choice(cli, c, cmd, argv, argc);
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
            ret = delete_data_node(y_case_n, argv[0], cli);
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
                    ret = add_data_node(leaf_next, optargs->value, cli);
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
    cli_print(cli, "incomplete command, please use '?' for options choice");
    return CLI_ERROR;
}

int cmd_yang_no_case(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    struct lysc_node *y_case_n;


    // case might be leaf or container, for leaf we get the child, for container the child is null and we use y_node
    y_case_n = (struct lysc_node *) lysc_node_child(y_node);
    if (y_case_n == NULL)
        y_case_n = y_node;

    int ret;

    // if the case node is leaf/leaf-list,we need to remove all leafs from data_node.
    if (y_case_n->nodetype == LYS_LEAF || y_case_n->nodetype == LYS_LEAFLIST) {
        struct lysc_node *leaf_next;
        LY_LIST_FOR(y_case_n, leaf_next) {
            ret = delete_data_node(leaf_next, NULL, cli);
            if (ret != LY_SUCCESS) {
                cli_print(cli, "Failed to create the yang data node for '%s'\n", y_case_n->name);
                return CLI_ERROR;
            }
        }
        return CLI_OK;
    }
    // data node is container.
    ret = delete_data_node(y_case_n, NULL, cli);
    if (ret != LY_SUCCESS) {
        cli_print(cli, "Failed to delete the yang data node '%s'\n", y_case_n->name);
        return CLI_ERROR;
    }
    return CLI_OK;

}


int register_cmd_choice_core(struct cli_def *cli, struct lysc_node *y_node, struct cli_command *parent_cmd,
                             struct cli_command *parent_no_cmd,
                             unsigned int mode) {
    /* ragisterning the choice is the most complex function, change carefully*/
    char help[100] = {0};
    char no_help[100] = {0};
    const struct lys_module *y_root_module = lysc_owner_module(y_node);

    sprintf(help, "configure %s (%s) [choice]", y_node->name, y_node->module->name);
    sprintf(no_help, "delete %s (%s) [choice]", y_node->name, y_node->module->name);

    char *cmd_hash = (char *) y_root_module->name;
    // this can be called recursively. so we might pass the mode.

    if (mode == -1)
        mode = y_get_curr_mode(y_node);
    struct cli_command *choice_cmd, *choice_no_cmd;

    // there is ietf-yang where choice and choice has same name.
    // we don't want to register this choice to avoid duplication.
    if (y_node->parent != NULL
        && y_node->parent->nodetype == LYS_CONTAINER
        && !strcmp(y_node->parent->name, y_node->name)) {
        choice_cmd = parent_cmd;
        choice_no_cmd = parent_no_cmd;
    } else {
        choice_cmd = cli_register_command(cli, parent_cmd, y_node, y_node->name,
                                          cmd_yang_choice, PRIVILEGE_UNPRIVILEGED,
                                          mode, cmd_hash, help);

        choice_no_cmd = cli_register_command(cli, parent_no_cmd, y_node, y_node->name,
                                             cmd_yang_no_choice, PRIVILEGE_UNPRIVILEGED,
                                             mode, cmd_hash, no_help);
    }


    struct lysc_node_choice *y_choice = (struct lysc_node_choice *) y_node;
    struct lysc_node *y_case;

    LY_LIST_FOR((struct lysc_node *) y_choice->cases, y_case) {
        sprintf(help, "configure %s (%s) [case]", y_case->name, y_case->module->name);
        sprintf(no_help, "delete %s (%s) [case]", y_case->name, y_case->module->name);
//        struct cli_command *case_cmd = choice_cmd;

        struct cli_command *case_cmd = cli_register_command(cli, choice_cmd, (void *) y_case, y_case->name,
                                                            cmd_yang_case, PRIVILEGE_UNPRIVILEGED,
                                                            mode, cmd_hash, help);

        struct cli_command *case_no_cmd = cli_register_command(cli, choice_no_cmd, (void *) y_case, y_case->name,
                                                               cmd_yang_no_case, PRIVILEGE_UNPRIVILEGED,
                                                               mode, cmd_hash, no_help);


        struct lysc_node *case_child_list = (struct lysc_node *) lysc_node_child(y_case);
        struct lysc_node *case_child;

        LY_LIST_FOR(case_child_list, case_child) {
            // if the case-child is another choice then run recursively
            if (case_child->nodetype == LYS_CHOICE) {
                // should be called with same mode.
                return register_cmd_choice_core(cli, case_child, case_cmd, case_no_cmd, mode);
            }
            if (case_child->nodetype == LYS_LEAF) {
                char *optarg_help;
                if (case_child->dsc != NULL)
                    optarg_help = strdup(case_child->dsc);
                else {
                    optarg_help = malloc(strlen(case_child->name) + strlen("configure ") + 2);
                    sprintf((char *) optarg_help, "configure %s", strdup(case_child->name));
                }

                struct cli_optarg *o = cli_register_optarg(case_cmd, case_child->name,
                                                           CLI_CMD_ARGUMENT,
                                                           PRIVILEGE_PRIVILEGED,
                                                           mode, optarg_help, optagr_get_compl_candidate,
                                                           yang_data_validator,
                                                           NULL);
                o->opt_model = (void *) case_child;

                free(optarg_help);
            } else
                register_commands_schema(case_child, cli);
        }
    }

    return 0;
}

int register_cmd_choice(struct cli_def *cli, struct lysc_node *y_node) {
    if (y_node->flags & LYS_CONFIG_R)
        return CLI_OK;
    int mode = -1;
    struct cli_command *parent_cmd = find_parent_cmd(cli, y_node);
    struct cli_command *parent_no_cmd = find_parent_no_cmd(cli, y_node);

    if (parent_cmd != NULL)
        mode = parent_cmd->mode;

    if (parent_no_cmd == NULL)
        parent_no_cmd = ((struct cli_ctx_data *) cli_get_context(cli))->no_cmd;

    return register_cmd_choice_core(cli, y_node, parent_cmd, parent_no_cmd, mode);
}