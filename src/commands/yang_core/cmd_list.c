//
// Created by ali on 10/19/23.
//
#include "src/utils.h"
#include "y_utils.h"
#include "yang_core.h"
#include "data_validators.h"
#include "data_factory.h"
#include "src/onm_logger.h"
#include "data_cmd_compl.h"

int cmd_print_list_order(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    int curr_indx = 10;
    struct lyd_node *next = NULL, *entry_child = NULL;
    struct lysc_node * y_node = (struct lysc_node * )c->cmd_model;
    struct lyd_node *list_entries = get_list_nodes(y_node);
    char line[265] = {'\0'};
    LY_LIST_FOR(list_entries, next) {
        struct lyd_node *entry_children = lyd_child(next);
        strcat(line, next->schema->name);
        LY_LIST_FOR(entry_children, entry_child) {
            if (lysc_is_key(entry_child->schema)) {
                strcat(line, " ");
                strcat(line, lyd_get_value(entry_child));
            }
        }
        cli_print(cli, "%s index [%d]", line, curr_indx);
        memset(line, '\0', 265);
        curr_indx += 10;
    }
    return EXIT_SUCCESS;

}

int cmd_yang_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, "  <cr>");
            cli_print(cli, "  delete");
            return CLI_OK;
        } else {
            cli_error(cli, "ERROR: unknown argument '%s'", argv[0]);
            return CLI_ERROR_ARG;
        }
    }

    struct cli_optarg_pair *optargs = cli->found_optargs;
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    int index = 0; // no index

    int is_delete = cli_get_optarg_value(cli, "delete", NULL) ? 1 : 0;
    char *idx_char = cli_get_optarg_value(cli, "index", NULL);
    if (idx_char != NULL) {
        char *idx_endptr;
        index = (int) strtol(idx_char, &idx_endptr, 10);

        // Check for conversion errors
        if ((*idx_endptr != '\0' && *idx_endptr != '\n') || (index == 0)) {
            cli_error(cli, "ERROR: <index> must be numeric greater than 0, entered value=%s", optargs->value);
            return CLI_ERROR;
        }
    }

    int ret;
    optargs = cli->found_optargs;
    create_argv_from_optpair(optargs, &argv, &argc);
    if (is_delete) {
        ret = delete_data_node_list(y_node, argv, argc, cli);
        if (ret != LY_SUCCESS) {
            LOG_ERROR("Failed to delete the data tree");
            cli_print(cli, "failed to execute command, error with adding the data node.");
            free_argv(argv,argc);
            return CLI_ERROR;
        }
        return CLI_OK;
    } else {
        ret = add_data_node_list(y_node, argv, argc, index, cli);
    }


    if (ret != LY_SUCCESS) {
        LOG_ERROR("Failed to create/delete the data tree");
        cli_print(cli, "failed to execute command, error with adding the data node.");
        free_argv(argv,argc);
        return CLI_ERROR;
    }

    char *mod_str;
    ssize_t mod_str_len = strlen(cmd) + strlen(argv[0]) + 3;
    for (int i = 0; i < argc; i++) {
        mod_str_len += strlen(argv[i]) + 2;
    }
    mod_str = malloc(mod_str_len);
    memset(mod_str, 0, mod_str_len);
    strcat(mod_str, (char *) cmd);
    for (int i = 0; i < argc; i++) {
        strcat(mod_str, "[");
        strcat(mod_str, argv[i]);
        strcat(mod_str, "]");
    }

    int mode = y_get_next_mode(y_node);

    cli_push_configmode(cli, mode, mod_str);
    free(mod_str);
    free_argv(argv,argc);
    return CLI_OK;
}

int register_cmd_list(struct cli_def *cli, struct lysc_node *y_node) {
    char help[100];
    unsigned int mode;

    const struct lys_module *y_root_module = lysc_owner_module(y_node);
    char *cmd_hash = (char *) y_root_module->name;

    sprintf(help, "configure %s (%s) [list]", y_node->name, y_node->module->name);

    mode = y_get_curr_mode(y_node);

    struct cli_command *c = cli_register_command(cli, NULL, y_node, y_node->name, cmd_yang_list,
                                                 PRIVILEGE_PRIVILEGED, mode, cmd_hash, help);

    const struct lysc_node *child_list = lysc_node_child(y_node);
    const struct lysc_node *child;
    struct cli_optarg *o;
//    char *list_cmd_help = create_list_cmd_help(y_node, lysc_is_userordered(y_node));

    LY_LIST_FOR(child_list, child) {
        if (lysc_is_key(child)) {
            const char *optarg_help;
            LY_DATA_TYPE type = ((struct lysc_node_leaf *) child)->type->basetype;
            if (type == LY_TYPE_IDENT)
                optarg_help = creat_help_for_identity_type((struct lysc_node *) child);
            else{
                if (child->dsc != NULL)
                    optarg_help = strdup(child->dsc);
                else {
                    optarg_help = malloc(strlen(child->name) + strlen("configure ") + 2);
                    sprintf((char*)optarg_help,"configure %s", strdup(child->name));
                }
            }

            o =cli_register_optarg(c, child->name, CLI_CMD_ARGUMENT, PRIVILEGE_PRIVILEGED,
                                mode, optarg_help, optagr_get_compl, yang_data_validator, NULL);
            o->opt_model = (void *)child; // for get_completion
            free((char*)optarg_help);
        }
    }
    cli_register_optarg(c, "delete", CLI_CMD_OPTIONAL_FLAG, PRIVILEGE_PRIVILEGED,
                        mode, "to delete the list entry", NULL, NULL, NULL);
    if (lysc_is_userordered(y_node)) {
        cli_register_optarg(c, "index", CLI_CMD_OPTIONAL_ARGUMENT, PRIVILEGE_PRIVILEGED,
                            mode, "to delete the list entry", NULL, NULL, NULL);

        // add print order command

        struct cli_command *print_order = cli_register_command(cli, NULL, NULL, "print-order", NULL,
                                                               PRIVILEGE_PRIVILEGED, mode, cmd_hash,
                                                               "print ordered entries of the list");

        cli_register_command(cli, print_order, y_node, y_node->name, cmd_print_list_order,
                             PRIVILEGE_PRIVILEGED, mode, cmd_hash,
                             "print ordered entries of the list");

    }


    return 0;
}
