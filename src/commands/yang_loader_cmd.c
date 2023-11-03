//
// Created by ali on 10/9/23.
//

/*
 * cli commands generator functions
 * */
#include "yang_loader_cmd.h"
#include "yang_core/yang_core.h"

extern struct ly_ctx *yang_ctx;

enum register_node_routine_signals {
    REG_NO_SIG = 0,
    REG_SKIP_NEXT_SIG,
    REG_DEF_SIG,
    REG_OK_SIG
};



int set_yang_searchdir(const char *dir) {
    printf("INFO:onm_yang.c: setting yang search path to `%s`\n",dir);
    ly_ctx_set_searchdir(yang_ctx, dir);
    return 0;
}
int unset_yang_searchdir(const char *dir) {
    printf("INFO:onm_yang.c: setting yang search path to `%s`\n",dir);
    ly_ctx_unset_searchdir(yang_ctx, dir);
    return 0;
}

const char * const* get_yang_searchdirs(){
    return ly_ctx_get_searchdirs(yang_ctx);
}


static int register_node_routine(struct cli_def *cli, struct lysc_node *schema) {
    if (schema->flags & LYS_CONFIG_R) {
        return REG_NO_SIG;
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
        case LYS_LEAFLIST:
            printf("TRACE: register CLI command for leaf-list: %s\r\n", schema->name);
            register_cmd_leaf_list(cli, schema);
            break;
        case LYS_CHOICE:
            printf("TRACE: register CLI command for choice: %s\r\n", schema->name);
            register_cmd_choice(cli, schema);
            return REG_SKIP_NEXT_SIG;
        default:
            return REG_DEF_SIG;
    }
    return REG_OK_SIG;
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
    int signal;
    LYSC_TREE_DFS_BEGIN(schema, child) {
            signal = register_node_routine(cli, child);
            if (signal == REG_SKIP_NEXT_SIG)
                LYSC_TREE_DFS_continue = 1;

        LYSC_TREE_DFS_END(schema->next, child);
    }
    printf("DEBUG:commands.c: schema `%s` registered successfully\r\n", schema->name);

}

static void unregister_node_routine(struct cli_def *cli, struct lysc_node *schema) {
    if (schema->flags & LYS_CONFIG_R) {
        return;
    }
    const struct lys_module *y_owner_module = lysc_owner_module(schema);
    cli_unregister_command(cli, strdup(schema->name), (char *) strdup(y_owner_module->name));
}

int unregister_commands_schema(struct lysc_node *schema, struct cli_def *cli) {

    struct lysc_node *child = NULL;

    LYSC_TREE_DFS_BEGIN(schema, child) {
            printf("DEBUG:commands.c: unregistering command for  `%s`\n", child->name);
            unregister_node_routine(cli, child);
        LYSC_TREE_DFS_END(schema->next, child);
    }
    printf("DEBUG:commands.c: schema `%s` unregistered successfully\r\n", schema->name);

}

int cmd_yang2cmd_remove(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (argc == 0) {
        cli_print(cli, "  please specify yang module name");
        return CLI_INCOMPLETE_COMMAND;
    }
    if (strcmp(argv[0], "?") == 0) {
        cli_print(cli, "  specify yang module name to generate commands from");
        return CLI_OK;
    }
    const char *all_features[] = {"*", NULL};
    char *module_name = (char *) argv[0];
    const struct lys_module *module = ly_ctx_load_module(yang_ctx, module_name, NULL, all_features);

    if (module == NULL) {
        cli_print(cli, "  ERROR: module `%s` not found, \n"
                       "  please make sure to set the correct search dir for yang,\n"
                       "  use command 'yang search_dir set /path/to/yang_modules' in enable mode", module_name);
        return CLI_ERROR;
    } else if (module->compiled == NULL) {
        cli_print(cli, "  ERROR: module `%s` was not complied check onm_cli logs for details\n", module_name);
        return CLI_ERROR;
    } else if (module->compiled->data == NULL) {
        if (module->parsed == NULL) {
            cli_print(cli, "  ERROR: module `%s` is loaded but not parsed.. check logs for details\n", module_name);
            return CLI_ERROR;
        } else if (module->parsed->augments != NULL) {
            cli_print(cli, "  WARN: module `%s` is loaded but not parsed as it's augmenting `%s`\n"
                           "  please run yang generate for the augmented module", module_name,
                      module->parsed->augments->node.name);
            return CLI_ERROR;
        }
    }

    unregister_commands_schema(module->compiled->data, cli);
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
    const char *all_features[] = {"*", NULL};
    char *module_name = (char *) argv[0];
    const struct lys_module *module = ly_ctx_load_module(yang_ctx, module_name, NULL, all_features);

    if (module == NULL) {
        cli_print(cli, "  ERROR: module `%s` not found, \n"
                       "  please make sure to set the correct search dir for yang,\n"
                       "  use command 'yang search_dir set /path/to/yang_modules' in enable mode", module_name);
        return CLI_ERROR;
    } else if (module->compiled == NULL) {
        cli_print(cli, "  ERROR: module `%s` was not complied check onm_cli logs for details\n", module_name);
        return CLI_ERROR;
    }

    // load all imported modules and register commands if any has
    struct lysp_import *imported_modules = module->parsed->imports;
    LY_ARRAY_COUNT_TYPE i;
    LY_ARRAY_FOR(imported_modules, i) {
        cli_print(cli, "  INFO: module `%s` importing`%s` module\n"
                       "  loading `%s` as well", module_name,
                  imported_modules[i].name, imported_modules[i].name);
        const char *new_argv = {imported_modules[i].name};
        cmd_yang2cmd_generate(cli, c, cmd, (char **) &new_argv, argc);
    }
    if (module->compiled->data == NULL)
        return CLI_OK;
    // if the module is already registered remove it first
    unregister_commands_schema(module->compiled->data, cli);

    register_commands_schema(module->compiled->data, cli);
    cli_print(cli, "  yang commands generated successfully for module=%s", module_name);
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

int cmd_yang_list_modules(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, "  <cr>");
            return CLI_MISSING_ARGUMENT;
        }
    }
    unsigned int index = 0;
    struct lys_module *mod;


    while ((mod = (struct lys_module *) ly_ctx_get_module_iter(yang_ctx, &index))) {
        if (mod != NULL)
            cli_print(cli, "[+] %s", mod->name);
    }
    return CLI_OK;
}

int yang_cmd_loader_init(struct cli_def *cli) {
    struct cli_command *yang_cmd = cli_register_command(cli, NULL, NULL,
                                                        "yang", NULL, PRIVILEGE_PRIVILEGED,
                                                        MODE_EXEC, NULL, "yang settings");
    if (yang_cmd == NULL)
        printf("failed\n");
    struct cli_command *yang_set_cmd = cli_register_command(cli, yang_cmd, NULL,
                                                            "set", NULL, PRIVILEGE_UNPRIVILEGED,
                                                            MODE_EXEC, NULL, "set yang settings");
    struct cli_command *yang_unset_cmd = cli_register_command(cli, yang_cmd, NULL,
                                                              "unset", NULL, PRIVILEGE_UNPRIVILEGED,
                                                              MODE_EXEC, NULL, "unset yang settings");
    cli_register_command(cli, yang_set_cmd, NULL,
                         "searchdir", cmd_yang_searchdir_set, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, NULL, "set yang search dir: yang set searchdir <path/to/modules>");
    cli_register_command(cli, yang_unset_cmd, NULL,
                         "searchdir", cmd_yang_searchdir_unset, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, NULL, "unset yang search dir: yang set searchdir <path/to/modules>");

    cli_register_command(cli, yang_cmd, NULL,
                         "list-seachdir", cmd_yang_list_searchdirs, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, NULL, "list yang serachdirs");

    cli_register_command(cli, yang_cmd, NULL,
                         "load-module", cmd_yang2cmd_generate, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, NULL, "load yang module to onm_cli:yang load-module <module-name>");

    cli_register_command(cli, yang_cmd, NULL,
                         "remove-module", cmd_yang2cmd_remove, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, NULL, "remove yang module from onm_cli:yang remove-module <module-name>");

    cli_register_command(cli, yang_cmd, NULL,
                         "list-modules", cmd_yang_list_modules, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, NULL, "list all the loaded yang modules.");

    return 0;
}