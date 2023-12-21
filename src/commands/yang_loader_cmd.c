//
// Created by ali on 10/9/23.
//

/*
 * cli commands generator functions
 * */
#include "yang_loader_cmd.h"
#include "yang_core/yang_core.h"
#include "src/onm_sysrepo.h"
#include "yang_core/y_utils.h"
#include "src/onm_logger.h"

extern struct ly_ctx *yang_ctx;

int set_yang_searchdir(const char *dir) {
    LOG_DEBUG("onm_yang.c: setting yang search path to `%s`", dir);
    ly_ctx_set_searchdir(yang_ctx, dir);
    return 0;
}

int unset_yang_searchdir(const char *dir) {
    LOG_DEBUG("onm_yang.c: setting yang search path to `%s`", dir);
    ly_ctx_unset_searchdir(yang_ctx, dir);
    return 0;
}

const char *const *get_yang_searchdirs() {
    return ly_ctx_get_searchdirs(yang_ctx);
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


int cmd_sysrepo_load_module(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    struct ly_ctx *sysrepo_ctx = (struct ly_ctx *) sysrepo_get_ctx();
    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, " <cr>");
            return CLI_MISSING_ARGUMENT;
        } else {
            cli_print(cli, " unknown argument %s", argv[0]);
            return CLI_ERROR_ARG;
        }
    }
    struct lys_module *mod;
    unsigned int index = 0;


    while ((mod = (struct lys_module *) ly_ctx_get_module_iter(sysrepo_ctx, &index))) {
        if (mod != NULL)
            mod2cmd_generate(cli, mod);
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
                         "list-modules", cmd_yang_list_modules, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, NULL, "list all the loaded yang modules.");


    struct cli_command *sysrepo_cmd = cli_register_command(cli, yang_cmd, NULL,
                                                           "sysrepo", NULL, PRIVILEGE_PRIVILEGED,
                                                           MODE_EXEC, NULL, "sysrepo commands");

    cli_register_command(cli, sysrepo_cmd, NULL,
                         "load-modules", cmd_sysrepo_load_module, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, NULL, "load and compile all yang modules from sysrepo: yang sysrepo load-modules");




    return 0;
}


