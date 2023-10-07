//
// Created by ali on 10/4/23.
//

#include "../utils.h"
#include "commands.h"
#include "yang_commands.h"


#ifdef __GNUC__
#define UNUSED(d) d __attribute__((unused))
#else
#define UNUSED(d) d
#endif


/*
 * Forward Declarations for cli default commands
 * */
static int cmd_frr(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);

static int cmd_exit(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);





/*
 * Globals
 * */
//struct term_mode_t {
//    int mode;
//    char *mode_desc;
//    struct term_mode_t *prev;
//} *term_mode;

enum {
    MODE_FRR = 10,

};


//int push_term_mode(int mode, char *desc) {
//    struct term_mode_t *c = malloc(sizeof(struct term_mode_t));
//    c->mode = mode;
//    c->mode_desc = desc;
//    c->prev = term_mode;
//    term_mode = c;
//}

/*
 * default commands definition
 * */


unsigned int regular_count = 0;
unsigned int debug_regular = 0;

int cmd_frr(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    cli_push_configmode(cli,MODE_FRR,"frr");
//    cli_set_transient_mode(cli,MODE_CONFIG);
//    push_term_mode(MODE_FRR, "frr");
    return CLI_OK;
}

int cmd_frr_router(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    cli_push_configmode(cli,MODE_FRR+1,"frr-router");
//    cli_set_transient_mode(cli, MODE_FRR);
//    push_term_mode(MODE_FRR, "frr");
    return CLI_OK;
}

//int cmd_exit(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
////    struct term_mode_t *term_prev;
////    if (term_mode->prev == NULL) {
////        return cli_exit(cli, c, cmd, argv, argc);
////    }
////
////    term_prev = term_mode->prev;
////    free(term_mode);
////    term_mode = term_prev;
////    cli_set_configmode(cli,cli->transient_mode,cli->buildmode);
//
//    printf("cmd_full_name=%s\n", cli->modestring);
//
////    cli_set_configmode(cli, c->parent->parent->mode, c->parent->parent->full_command_name);
//
//    return CLI_OK;
//}

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
    // init term mode
//    term_mode = malloc(sizeof(struct term_mode_t));
//    term_mode->mode = MODE_CONFIG;
//    term_mode->prev = NULL;

//    cli_set_auth_callback(cli, check_auth);
//    cli_register_command(cli, NULL, NULL, "exit",
//                         cmd_exit, PRIVILEGE_UNPRIVILEGED,
//                         MODE_ANY, "exit to prev mode");


    cli_register_command(cli, NULL, NULL,
                         "frr", cmd_frr, PRIVILEGE_UNPRIVILEGED,
                         MODE_CONFIG, "frr subsystem config");

    cli_register_command(cli, NULL, NULL,
                         "router", cmd_frr_router, PRIVILEGE_UNPRIVILEGED,
                         MODE_FRR, "frr subsystem config");
    printf("hello\n");
    // yang commands
//    yang_cmds_init(cli);




}
