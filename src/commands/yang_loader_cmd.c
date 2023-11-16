//
// Created by ali on 10/9/23.
//

/*
 * cli commands generator functions
 * */
#include "yang_loader_cmd.h"
#include "yang_core/yang_core.h"

extern struct ly_ctx *yang_ctx;
extern struct lyd_node *root_data;


int set_yang_searchdir(const char *dir) {
    printf("INFO:onm_yang.c: setting yang search path to `%s`\n", dir);
    ly_ctx_set_searchdir(yang_ctx, dir);
    return 0;
}

int unset_yang_searchdir(const char *dir) {
    printf("INFO:onm_yang.c: setting yang search path to `%s`\n", dir);
    ly_ctx_unset_searchdir(yang_ctx, dir);
    return 0;
}

const char *const *get_yang_searchdirs() {
    return ly_ctx_get_searchdirs(yang_ctx);
}


int cmd_yang_remove_module(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (argc == 0) {
        cli_print(cli, "  please specify yang module name");
        return CLI_INCOMPLETE_COMMAND;
    }
    if (strcmp(argv[0], "?") == 0) {
        cli_print(cli,
                  "  specify yang module name to load,\n NOTE: this will not generate the command until `yang compile`"
                  "cmd is executed, and it will compile all loaded modules.");
        return CLI_OK;
    }
    if (strcmp(argv[0], "all") == 0) {
        unsigned int index = 0;
        struct lys_module *mod;
        while ((mod = (struct lys_module *) ly_ctx_get_module_iter(yang_ctx, &index))) {
            if (mod != NULL) {
                const char *new_argv[1] = {mod->name};
                cmd_yang_remove_module(cli, c, cmd, (char **) new_argv, 1);
            }
        }
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


int cmd_yang_load_module(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    printf("DEBUG:commands.c:cmd_yang2cmd_generate(): executing command `%s`\n", cmd);

    if (argc == 0) {
        cli_print(cli, "  please specify yang module name");
        return CLI_INCOMPLETE_COMMAND;
    }
    if (strcmp(argv[0], "?") == 0) {
        cli_print(cli, "  specify yang module name to remove or `all` to remove all modules");
        return CLI_OK;
    }


    const char *all_features[] = {"*", NULL};
    char *module_name = (char *) argv[0];
    if (ly_ctx_load_module(yang_ctx, module_name, NULL, all_features) == NULL) {
        cli_print(cli, "  ERROR: module `%s` not found, \n"
                       "  please make sure to set the correct search dir for yang,\n"
                       "  use command 'yang search_dir set /path/to/yang_modules' in enable mode", module_name);
        return CLI_ERROR;
    }
    cli_print(cli, "  module loaded successfully!\n  "
                   "make sure to run `yang compile` command after done with loading all required modules.");
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

int mod2cmd_generate(struct cli_def *cli, const struct lys_module *module) {
    // skip the none implemented modules
    if (!module->implemented) {
        return -1;
    }
    if (module->compiled->data == NULL)
        return CLI_OK;
    // if the module is already registered remove it first

    unregister_commands_schema(module->compiled->data, cli);

    register_commands_schema(module->compiled->data, cli);
    cli_print(cli, "  yang commands generated successfully for module=%s", module->name);
    return CLI_OK;
}

int cmd_yang_compile(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    ly_err_clean(yang_ctx, NULL);
    if (ly_ctx_compile(yang_ctx) != LY_SUCCESS) {
        cli_print(cli, "compile failed, reason=%s", ly_errmsg(yang_ctx));
        return CLI_ERROR;
    }
    unsigned int index = 0;
    struct lys_module *mod;


    while ((mod = (struct lys_module *) ly_ctx_get_module_iter(yang_ctx, &index))) {
        if (mod != NULL)
            mod2cmd_generate(cli, mod);
    }
    return CLI_OK;


}

int cmd_print_local_config(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (root_data==NULL){
        cli_print(cli,"no new config yet!");
        return CLI_ERROR;
    }
    char *result;
    lyd_print_mem(&result, root_data, LYD_XML, 0);
    cli_print(cli, result, NULL);
    return CLI_OK;
}

int yang_cmd_loader_init(struct cli_def *cli) {
    struct cli_command *yang_cmd = cli_register_command(cli, NULL, NULL,
                                                        "yang", NULL, PRIVILEGE_PRIVILEGED,
                                                        MODE_EXEC, NULL, "yang settings");
    if (yang_cmd == NULL)
        printf("failed\n");

    struct cli_command *print = cli_register_command(cli, NULL, NULL,
                                                            "print", NULL, PRIVILEGE_UNPRIVILEGED,
                                                            MODE_ANY, NULL, "print the candidate/running config");

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
                         "load-module", cmd_yang_load_module, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, NULL, "load yang module to onm_cli:yang load-module <module-name>\n "
                                          " make sure to run yang compile after you load all the required modules.");

    cli_register_command(cli, yang_cmd, NULL,
                         "remove-module", cmd_yang_remove_module, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, NULL,
                         "remove yang module from onm_cli:yang remove-module <module-name>|all");


    cli_register_command(cli, yang_cmd, NULL,
                         "list-modules", cmd_yang_list_modules, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, NULL, "list all the loaded yang modules.");

    cli_register_command(cli, yang_cmd, NULL,
                         "compile", cmd_yang_compile, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, NULL, "compile loaded modules, use ");


    cli_register_command(cli, print, NULL,
                         "local-candidate-config", cmd_print_local_config, PRIVILEGE_UNPRIVILEGED,
                         MODE_ANY, NULL, "print the local new config data tree");
    cli_register_command(cli, print, NULL,
                         "cdb-candidate-config", NULL, PRIVILEGE_UNPRIVILEGED,
                         MODE_ANY, NULL, "print cdp candidate config");

    cli_register_command(cli, print, NULL,
                         "cdb-running-config", NULL, PRIVILEGE_UNPRIVILEGED,
                         MODE_ANY, NULL, "print cdp running config");



    return 0;
}


