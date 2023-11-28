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

    int key_count = get_keys_count(y_node);
    if (argc != key_count) {
        cli_print(cli, "ERROR: please enter %d Key(s) for the list entry of %s", key_count, cmd);
        return CLI_MISSING_ARGUMENT;
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
    ret = add_data_node_list(y_node, c, argv, argc);
    if (ret != LY_SUCCESS) {
        fprintf(stderr, "Failed to create the data tree\n");
        cli_print(cli, "failed to execute command, error with adding the data node.");
        return CLI_ERROR;
    }

    char *mod_str;
    ssize_t mod_str_len = strlen(cmd) + strlen(argv[0]) + 3;
    for (int i = 0; i < argc; i++) {
        mod_str_len += strlen(argv[i]) + 2;
    }
    mod_str = malloc(mod_str_len);
    memset(mod_str,0,mod_str_len);
    strcat(mod_str,(char *) cmd);
    for (int i = 0; i < argc; i++) {
        strcat(mod_str,"[");
        strcat(mod_str,argv[i]);
        strcat(mod_str,"]");
    }

    int mode = y_get_next_mode(y_node);

    cli_push_configmode(cli, mode, mod_str);
    return CLI_OK;
}


char *create_list_cmd_help(struct lysc_node *y_node) {
    const struct lysc_node *child_list = lysc_node_child(y_node);
    const struct lysc_node *child;

    // Calculate the total length needed for the string
    size_t total_len = strlen(y_node->name) + 3; // +3 for "> ", space, and null terminator

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


    return list_cmd_help;
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
    struct cli_optarg *o;
    char *list_cmd_help = create_list_cmd_help(y_node);
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
