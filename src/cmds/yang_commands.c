//
// Created by ali on 10/7/23.
//

#include "yang_commands.h"
#include "../utils.h"

static int cmd_yang_container(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);

static int cmd_yang_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);

static int cmd_yang_leaf(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);

static int cmd_yang2cmd_generate(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);

static int cmd_yang_path(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);


/*
 * cli commands generator functions
 * */

/**
 * register command in cli for a container node
 * @param name
 * @param parent_name
 * @param cli
 * @return
 */
int register_cmd_container(struct cli_def *cli, struct lysc_node *y_node) {
    char help[100];
    sprintf(help, "configure setting for %s", y_node->name);
    unsigned int mode;
    if (y_node->parent == NULL)
        mode = MODE_CONFIG;
    else
        mode = str2int_hash((char *) y_node->parent->name);
    cli_register_command(cli, NULL, y_node, y_node->name, cmd_yang_container, PRIVILEGE_UNPRIVILEGED, mode,
                         help);

}

/**
 * register command in cli for a leaf node
 * @param name
 * @param parent_name
 * @param cli
 * @return
 */
int register_cmd_leaf(struct cli_def *cli, struct lysc_node *y_node) {
    char help[100];
    sprintf(help, "configure %s", y_node->name);
    unsigned int mode;
    if (y_node->parent == NULL)
        mode = MODE_CONFIG;
    else
        mode = str2int_hash((char *) y_node->parent->name);
    struct cli_command *c = cli_register_command(cli, NULL, y_node, y_node->name, cmd_yang_leaf,
                                                 PRIVILEGE_PRIVILEGED, mode, help);
    cli_register_optarg(c, "value", CLI_CMD_ARGUMENT, PRIVILEGE_PRIVILEGED, mode,
                        y_node->dsc, NULL, NULL, NULL);

    return 0;
}

int register_cmd_list(struct cli_def *cli, struct lysc_node *y_node) {
    char help[100];
    sprintf(help, "configure %s", y_node->name);
    unsigned int mode;
    if (y_node->parent == NULL)
        mode = MODE_CONFIG;
    else
        mode = str2int_hash((char *) y_node->parent->name);
    struct cli_command *c = cli_register_command(cli, NULL, y_node, y_node->name, cmd_yang_list,
                                                 PRIVILEGE_PRIVILEGED, mode, help);

    const struct lysc_node *child_list = lysc_node_child(y_node);
    const struct lysc_node *child;
    LYSC_TREE_DFS_BEGIN(child_list, child) {
            if (child->flags & LYS_KEY) {
                cli_register_optarg(c, child->name, CLI_CMD_ARGUMENT, PRIVILEGE_PRIVILEGED, mode,
                                    child->dsc, NULL, NULL, NULL);
                break;
            }
        LYSC_TREE_DFS_END(child_list->next, child);
    }


    return 0;
}

static  void register_node_routine(struct cli_def * cli, struct lysc_node *schema){
    if (schema->flags & LYS_CONFIG_R) {
        return ;
    }
    switch (schema->nodetype) {
        case LYS_CONTAINER:
            printf("TRACE: register CLI command for container: %s\r\n", schema->name);
            register_cmd_container(cli, schema);
            break;
        case LYS_LEAF:
            printf("TRACE: register CLI command for leaf: %s\r\n", schema->name);
            register_cmd_leaf(cli, schema);
            break;
        case LYS_LIST:
            printf("TRACE: register CLI command for list: %s\r\n", schema->name);
            register_cmd_list(cli, schema);
            break;
            // Add cases for other node types as needed
        default:
            return ;
    }
    return ;
}

/**
 * register  commands from a yang schema.
 * @param schema  lysc_node schema the root node
 * @param cli     libcli cli
 * @return
 */
int register_commands_schema(struct lysc_node *schema, struct cli_def *cli) {
    printf("DEBUG:commands.c: registering schema for  `%s`\n", schema->name);
    struct lysc_node *child = NULL;
    LYSC_TREE_DFS_BEGIN(schema, child) {
            register_node_routine(cli,child);
        LYSC_TREE_DFS_END(schema->next, child);
    }
    printf("DEBUG:commands.c: schema `%s` registered successfully\r\n", schema->name);


}


int cmd_yang_leaf(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    printf("DEBUG:commands.c:cmd_yang_leaf(): executing command `%s`\r\n", cmd);
    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            return CLI_INCOMPLETE_COMMAND;
        } else {
            struct lysc_node *ne = (struct lysc_node *) c->cmd_model;
            char xpath[100];

            lysc_path(ne, LYSC_PATH_DATA, xpath, 100);


            if (ne != NULL)
                cli_print(cli, "  this command is for module=%s , node=%s, xpath=%s\r\n", ne->module->name, ne->name,
                          xpath);
            else
                cli_print(cli, "  failed to fine yang module\r\n");
            return CLI_OK;
        }

    }
    return CLI_MISSING_ARGUMENT;
}

int cmd_yang_container(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    printf("DEBUG:commands.c:cmd_yang_container(): executing command `%s`\n", cmd);
    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, "  %s", c->help);
            return CLI_INCOMPLETE_COMMAND;
        }
        cli_print(cli, "  unknown args\n");
        return CLI_ERROR_ARG;
    }


    int mode = str2int_hash((char *) cmd);
    cli_push_configmode(cli, mode, (char *) cmd);
    return CLI_OK;
}

int cmd_yang_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    printf("DEBUG:commands.c:cmd_yang_container(): executing command `%s` , help=%s\n", cmd,
           (char *) cli->user_context);
    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, "  key is missing for command %s\n", cmd);
            return CLI_INCOMPLETE_COMMAND;
        }
        cli_print(cli, "  configuring %s %s\n", cmd, argv[0]);
        return CLI_OK;
    }

    struct lysc_node *ne = (struct lysc_node *) c->cmd_model;
    char xpath[100];

    lysc_path(ne, LYSC_PATH_DATA, xpath, 100);


    if (ne != NULL)
        cli_print(cli, "  this command is for module=%s , node=%s, xpath=%s", ne->module->name, ne->name, xpath);
    else
        cli_print(cli, "  failed to fine yang module");

    int mode = str2int_hash((char *) cmd);
    cli_push_configmode(cli, mode, (char *) cmd);
    return CLI_OK;
}

int cmd_yang2cmd_generate(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    printf("DEBUG:commands.c:cmd_yang2cmd_generate(): executing command `%s`\n", cmd);

    if (argc == 0) {
        cli_print(cli, "  please specify yang module name");
        return CLI_INCOMPLETE_COMMAND;
    }
    if (strcmp(argv[0], "?") == 0) {
        cli_print(cli, "  specify yang module name to generate commands from");
        return CLI_OK;
    }
    char *module_name = (char *) argv[0];
    const struct lys_module *module =get_module_schema(module_name) ;
    if (module == NULL){
        cli_print(cli, "  ERROR: module `%s` not found, \n"
                       "  please make sure to set the correct search dir for yang,\n"
                       "  use command 'yang search_dir set /path/to/yang_modules' in enable mode",module_name);
        return CLI_ERROR;
    } else if (module->compiled == NULL){
        cli_print(cli, "  ERROR: module `%s` was not complied check onm_cli logs for details\n",module_name);
        return CLI_ERROR;
    } else if (module->compiled->data == NULL){
        if (module->parsed == NULL){
            cli_print(cli, "  ERROR: module `%s` is loaded but not parsed.. check logs for details\n",module_name);
            return CLI_ERROR;
        } else if (module->parsed->augments != NULL){
            cli_print(cli, "  WARN: module `%s` is loaded but not parsed as it's augmenting `%s`\n"
                           "  please run yang generate for the augmented module",module_name,module->parsed->augments->node.name);
            return CLI_ERROR;

        }

    }

    cli_print(cli, "  generating commands for yang module `%s`", module_name);
    register_commands_schema(module->compiled->data, cli);
    cli_print(cli, "  commands for yang module `%s` generated successfully", module_name);
    return CLI_OK;

}

int cmd_yang_searchdir_set(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (argc == 0) {
        cli_print(cli, "  specify yang dir");
        return CLI_INCOMPLETE_COMMAND;
    }
    if (strcmp(argv[0], "?") == 0) {
        cli_print(cli, "  yang dir was not specified");
        return CLI_MISSING_ARGUMENT;
    }

    set_yang_searchdir(argv[0]);
    return CLI_OK;

}

int cmd_yang_searchdir_unset(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (argc == 0) {
        cli_print(cli, "  specify yang dir");
        return CLI_INCOMPLETE_COMMAND;
    }
    if (strcmp(argv[0], "?") == 0) {
        cli_print(cli, "  yang dir was not specified");
        return CLI_MISSING_ARGUMENT;
    }

    unset_yang_searchdir(argv[0]);
    return CLI_OK;

}

int cmd_yang_list_searchdirs(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, "  <cr>");
            return CLI_MISSING_ARGUMENT;
        }
    }

    const char *const *dir_list = get_yang_searchdirs();
    while (*dir_list != NULL) {
        cli_print(cli, "  [+] %s", *dir_list);
        ++dir_list;
    }


}

int yang_cmds_init(struct cli_def *cli) {
    struct cli_command *yang_cmd = cli_register_command(cli, NULL, NULL,
                                                        "yang", NULL, PRIVILEGE_PRIVILEGED,
                                                        MODE_EXEC, "yang settings");
    if (yang_cmd == NULL)
        printf("failed\n");
    struct cli_command *yang_set_cmd = cli_register_command(cli, yang_cmd, NULL,
                                                            "set", NULL, PRIVILEGE_UNPRIVILEGED,
                                                            MODE_EXEC, "set yang settings");
    struct cli_command *yang_unset_cmd = cli_register_command(cli, yang_cmd, NULL,
                                                              "unset", NULL, PRIVILEGE_UNPRIVILEGED,
                                                              MODE_EXEC, "unset yang settings");
    cli_register_command(cli, yang_set_cmd, NULL,
                         "searchdir", cmd_yang_searchdir_set, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, "set yang search dir: yang set searchdir <path/to/modules>");
    cli_register_command(cli, yang_unset_cmd, NULL,
                         "searchdir", cmd_yang_searchdir_unset, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, "unset yang search dir: yang set searchdir <path/to/modules>");

    cli_register_command(cli, yang_cmd, NULL,
                         "generate-commands", cmd_yang2cmd_generate, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, "generate commands from module:yang generate-commands <module-name>");
    cli_register_command(cli, yang_cmd, NULL,
                         "list-seachdir", cmd_yang_list_searchdirs, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, "list yang serachdirs");
}