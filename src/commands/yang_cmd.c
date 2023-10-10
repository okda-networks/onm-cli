//
// Created by ali on 10/7/23.
//

#include "yang_cmd.h"
#include "../utils.h"


int cmd_yang_leaf(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    printf("DEBUG:commands.c:cmd_yang_leaf(): executing command `%s`\r\n", cmd);
    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            return CLI_INCOMPLETE_COMMAND;
        }
    }
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


