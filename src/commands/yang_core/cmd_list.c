//
// Created by ali on 10/19/23.
//

#include "y_utils.h"
#include "yang_core.h"
#include "data_validators.h"
#include "data_factory.h"


int get_keys_count(struct lysc_node *y_node) {
    int count = 0;
    const struct lysc_node *child_list = lysc_node_child(y_node);
    const struct lysc_node *child;
    LY_LIST_FOR(child_list, child) {
        if (lysc_is_key(child)) {
            count++;
        }
    }
    return count;
}

int cmd_yang_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (argc == 0) {
        cli_print(cli, "ERROR: please enter %s of %s entry", c->optargs->name, cmd);
        return CLI_MISSING_ARGUMENT;
    }


    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    uint8_t is_delete = 0;
    int index = 0; // no index

    int key_count = get_keys_count(y_node);
    if (argc < key_count) {
        cli_print(cli, "ERROR: please enter %d Key(s) for the list entry of %s", key_count, cmd);
        return CLI_MISSING_ARGUMENT;
    }

    // check if last arg is 'delete'
    if (strcmp(argv[argc - 1], "delete") == 0)
        is_delete = 1;

    if (lysc_is_userordered(y_node)) {
        // if this is true then argv[key_count] is the order <index>
        if ((key_count + 1 == argc && !is_delete) || (key_count + 2 == argc && is_delete)) {

            char *endptr; // Used to detect conversion errors
            index = (int) strtol(argv[key_count ], &endptr, 10);

            // Check for conversion errors
            if (*endptr != '\0' && *endptr != '\n' || (index ==0) ) {
                cli_print(cli, "ERROR: <index> must be numeric greater than 0, entered value=%s", argv[key_count]);
                return CLI_ERROR;
            }
        } // if no index we add the entry  to the end of the list.
    }


    // validate
    const struct lysc_node *child_list = lysc_node_child(y_node);
    const struct lysc_node *child;
    int arg_pos = -1;
    LY_LIST_FOR(child_list, child) {
        if (lysc_is_key(child)) {
            arg_pos++;
            //  validate data
            if (yang_data_validator(cli, cmd, argv[arg_pos], (void *) child) != CLI_OK)
                return CLI_ERROR_ARG;
        }
    }

    int ret;
    if (is_delete) {
        ret = delete_data_node_list(y_node, argv, argc);
        if (ret != LY_SUCCESS) {
            fprintf(stderr, "Failed to delete the data tree\n");
            cli_print(cli, "failed to execute command, error with adding the data node.");
            return CLI_ERROR;
        }
        return CLI_OK;
    } else
        ret = add_data_node_list(y_node, argv, argc,index);

    if (ret != LY_SUCCESS) {
        fprintf(stderr, "Failed to create/delete the data tree\n");
        cli_print(cli, "failed to execute command, error with adding the data node.");
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
    return CLI_OK;
}


char *create_list_cmd_help(struct lysc_node *y_node, uint8_t is_userordered) {
    const struct lysc_node *child_list = lysc_node_child(y_node);
    const struct lysc_node *child;

    // Calculate the total length needed for the string
    size_t total_len = strlen(y_node->name) + 11; // +3 for "> ", space, and null terminator, 8 for \s<index>

    LY_LIST_FOR(child_list, child) {
        if (lysc_is_key(child)) {
            total_len += strlen(child->name) + 3; // +3 for "<>", space
        }
    }

    // Allocate memory for the string
    char *list_cmd_help = (char *) malloc(total_len);
    if (!list_cmd_help) {
        perror("Memory allocation failed");
        return NULL;
    }

    // Construct the string
    strcpy(list_cmd_help, "> ");
    strcat(list_cmd_help, y_node->name);

    LY_LIST_FOR(child_list, child) {
        if (lysc_is_key(child)) {
            strcat(list_cmd_help, " <");
            strcat(list_cmd_help, child->name);
            strcat(list_cmd_help, ">");
        }
    }

    if (is_userordered)
        strcat(list_cmd_help, " <index>");


    return list_cmd_help;
}

int register_cmd_list(struct cli_def *cli, struct lysc_node *y_node) {
    char help[100];
    unsigned int mode;

    const struct lys_module *y_root_module = lysc_owner_module(y_node);
    char *cmd_hash = strdup(y_root_module->name);;

    sprintf(help, "configure %s (%s) [list]", y_node->name, y_node->module->name);

    mode = y_get_curr_mode(y_node);

    struct cli_command *c = cli_register_command(cli, NULL, y_node, y_node->name, cmd_yang_list,
                                                 PRIVILEGE_PRIVILEGED, mode, cmd_hash, help);

    const struct lysc_node *child_list = lysc_node_child(y_node);
    const struct lysc_node *child;
    struct cli_optarg *o;
    char *list_cmd_help = create_list_cmd_help(y_node, lysc_is_userordered(y_node));
    o = cli_register_optarg(c, "key(s)", CLI_CMD_ARGUMENT | CLI_CMD_DO_NOT_RECORD, PRIVILEGE_PRIVILEGED,
                            mode, list_cmd_help, NULL, NULL, NULL);
    LY_LIST_FOR(child_list, child) {
        if (lysc_is_key(child)) {
            const char *optarg_help;
            LY_DATA_TYPE type = ((struct lysc_node_leaf *) child)->type->basetype;
            if (type == LY_TYPE_IDENT)
                optarg_help = creat_help_for_identity_type((struct lysc_node *) child);
            else
                optarg_help = child->dsc;

            cli_optarg_addhelp(o, child->name, optarg_help);

        }
    }
    return 0;
}
