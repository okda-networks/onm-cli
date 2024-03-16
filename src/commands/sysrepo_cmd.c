/* SPDX-License-Identifier: AGPL-3.0-or-later */
/*
 * Authors:     Ali Aqrabawi, <aaqrbaw@okdanetworks.com>
 *
 *              This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU Affero General Public
 *              License Version 3.0 as published by the Free Software Foundation;
 *              either version 3.0 of the License, or (at your option) any later
 *              version.
 *
 * Copyright (C) 2024 Okda Networks, <aaqrbaw@okdanetworks.com>
 */

#include "sysrepo_cmd.h"
#include "yang_core/yang_core.h"
#include "src/onm_sysrepo.h"
#include "yang_core/y_utils.h"
#include "src/onm_logger.h"


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


int cmd_sysrepo_set_module_path(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    char *path = cli_get_optarg_value(cli, "absolute-path", NULL);
    sysrepo_set_module_path(path);
    return CLI_OK;
}


int cmd_sysrepo_list_modules(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    struct ly_ctx *sysrepo_ctx = (struct ly_ctx *) sysrepo_get_ctx();
    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, " <cr>");
            sysrepo_release_ctx();
            return CLI_MISSING_ARGUMENT;
        } else {
            cli_print(cli, " unknown argument %s", argv[0]);
            sysrepo_release_ctx();
            return CLI_ERROR_ARG;
        }
    }
    struct lys_module *mod;
    unsigned int index = 0;


    while ((mod = (struct lys_module *) ly_ctx_get_module_iter(sysrepo_ctx, &index))) {
        if (mod != NULL)
            cli_print(cli, "[+] %s", mod->name);
    }
    sysrepo_release_ctx();

    return CLI_OK;
}

int cmd_sysrepo_load_module(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    struct ly_ctx *sysrepo_ctx = (struct ly_ctx *) sysrepo_get_ctx();
    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, " <cr>");
            sysrepo_release_ctx();
            return CLI_MISSING_ARGUMENT;
        } else {
            cli_print(cli, " unknown argument %s", argv[0]);
            sysrepo_release_ctx();
            return CLI_ERROR_ARG;
        }
    }
    struct lys_module *mod;
    unsigned int index = 0;


    while ((mod = (struct lys_module *) ly_ctx_get_module_iter(sysrepo_ctx, &index))) {
        if (mod != NULL)
            mod2cmd_generate(cli, mod);
    }
    sysrepo_release_ctx();

    return CLI_OK;
}

int cmd_sysrepo_install_module(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    char *file = cli_get_optarg_value(cli, "module-file", NULL);
    if (sysrepo_insmod(file) != EXIT_SUCCESS)
        return CLI_ERROR;
    return CLI_OK;
}

int cmd_sysrepo_remove_module(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {


    char *file = cli_get_optarg_value(cli, "module-name", NULL);
    char *force = cli_get_optarg_value(cli, "force", NULL);

    if (sysrepo_rmmod(file, force ? 1 : 0) != EXIT_SUCCESS)
        return CLI_ERROR;
    return CLI_OK;
}

int sysrepo_commands_init(struct cli_def *cli) {


    struct cli_command *sysrepo_cmd = cli_register_command(cli, NULL, NULL,
                                                           "sysrepo", NULL, PRIVILEGE_PRIVILEGED,
                                                           MODE_EXEC, NULL, "sysrepo commands");

    struct cli_command *sysrepo_set_path = cli_register_command(cli, sysrepo_cmd, NULL,
                                                                "set-module-path", cmd_sysrepo_set_module_path,
                                                                PRIVILEGE_PRIVILEGED,
                                                                MODE_EXEC, NULL,
                                                                "set yang modules path search to install in sysrepo");

    cli_register_optarg(sysrepo_set_path, "absolute-path", CLI_CMD_ARGUMENT, PRIVILEGE_PRIVILEGED, MODE_EXEC,
                        "full absolute path to yang modules", NULL, NULL, NULL);

    cli_register_command(cli, sysrepo_cmd, NULL,
                         "list-modules", cmd_sysrepo_list_modules, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, NULL, "list all sysrepo yang modules");

    cli_register_command(cli, sysrepo_cmd, NULL,
                         "load-modules", cmd_sysrepo_load_module, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, NULL, "load all yang modules from sysrepo and generate the cmds.");

    struct cli_command *sysrepo_inst_mod = cli_register_command(cli, sysrepo_cmd, NULL,
                                                                "install-module", cmd_sysrepo_install_module,
                                                                PRIVILEGE_PRIVILEGED,
                                                                MODE_EXEC, NULL, "install yang module in sysrepo");
    cli_register_optarg(sysrepo_inst_mod, "module-file", CLI_CMD_ARGUMENT, PRIVILEGE_PRIVILEGED, MODE_EXEC,
                        "module yang file .yang", NULL, NULL, NULL);


    struct cli_command *sysrepo_rm_mod = cli_register_command(cli, sysrepo_cmd, NULL,
                                                              "remove-module", cmd_sysrepo_remove_module,
                                                              PRIVILEGE_PRIVILEGED,
                                                              MODE_EXEC, NULL, "remove yang module from sysrepo");

    cli_register_optarg(sysrepo_rm_mod, "module-name", CLI_CMD_ARGUMENT, PRIVILEGE_PRIVILEGED, MODE_EXEC,
                        "module name ex:ietf-vrrp", NULL, NULL, NULL);

    cli_register_optarg(sysrepo_rm_mod, "force", CLI_CMD_OPTIONAL_FLAG, PRIVILEGE_PRIVILEGED, MODE_EXEC,
                        "If there are other installed modules depending on module_name, remove them, too", NULL, NULL,
                        NULL);


    return 0;
}


