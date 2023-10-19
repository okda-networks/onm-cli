//
// Created by ali on 10/4/23.
//

#include "../utils.h"
#include "cmd.h"
#include "yang_cmd_loader.h"


#ifdef __GNUC__
#define UNUSED(d) d __attribute__((unused))
#else
#define UNUSED(d) d
#endif


/*
 * Forward Declarations for cli default commands
 * */
static int cmd_frr(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);


/*
 * Globals
 * */

enum {
    MODE_FRR = 10,

};


/*
 * default commands definition
 * */


unsigned int regular_count = 0;
unsigned int debug_regular = 0;

int cmd_frr(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    cli_push_configmode(cli, MODE_FRR, "frr");
    return CLI_OK;
}

int cmd_frr_router(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    cli_push_configmode(cli, MODE_FRR + 1, "frr-router");
    return CLI_OK;
}

int cmd_regular_callback(struct cli_def *cli) {
    regular_count++;
    if (debug_regular) {
        cli_print(cli, "Regular callback - %u times so far", regular_count);
        cli_reprompt(cli);
    }
    return CLI_OK;
}


int check_auth(const char *username, const char *password) {
    if (strcasecmp(username, USERNAME) != 0) return CLI_ERROR;
    if (strcasecmp(password, PASSWORD) != 0) return CLI_ERROR;
    return CLI_OK;
}


int onm_commands_init(struct cli_def *cli) {
    printf("INFO:commands.c: initializing commands\n");


    cli_set_auth_callback(cli, check_auth);


    cli_register_command(cli, NULL, NULL,
                         "frr", cmd_frr, PRIVILEGE_UNPRIVILEGED,
                         MODE_CONFIG,NULL,  "frr subsystem config");


    // initialize yang-cmd generator commands
    yang_cmd_loader_init(cli);


}
