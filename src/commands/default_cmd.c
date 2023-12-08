//
// Created by ali on 10/4/23.
//

#include "src/utils.h"
#include "default_cmd.h"
#include "src/onm_sysrepo.h"
#include "yang_core/data_factory.h"


#ifdef __GNUC__
#define UNUSED(d) d __attribute__((unused))
#else
#define UNUSED(d) d
#endif

extern struct lyd_node *root_data, *parent_data;

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

int cmd_discard_changes(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (root_data != NULL){
        lyd_free_all(root_data);
        root_data = NULL;
        if (sysrepo_discard_changes() != SR_ERR_OK){
            cli_print(cli, "failed to discard changes, sysrepo error!");
            return CLI_ERROR;
        }
        cli_print(cli, "config changes discarded!");
    } else {
        cli_print(cli, "no config changes found!");
    }
    return CLI_OK;
}

int cmd_exit2(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    // we need to shift the parent_data backward with each exit call.
    if (parent_data != NULL) {
        parent_data = (struct lyd_node *) parent_data->parent;
    }
    if (cli->term_mode_stack->prev != NULL && cli->term_mode_stack->prev->mode == MODE_CONFIG && root_data != NULL) {
        cli_print(cli, "ERROR: there are uncommitted changes, please commit them or discard them before exist");
        return CLI_ERROR;
    }
    return cli_exit(cli, c, cmd, argv, argc);
}

int cmd_print_local_config(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (root_data == NULL) {
        cli_print(cli, "no new config yet!");
        return CLI_ERROR;
    }
    char *result;
    lyd_print_mem(&result, root_data, LYD_XML, 0);
    cli_print(cli, result, NULL);
    return CLI_OK;
}

int cmd_commit(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    struct data_tree *config_dtree= get_config_data_tree();

    // commit changes.
    if (root_data == NULL) {
        cli_print(cli, " no modification to commit!");
        return CLI_OK;
    }
    if (sysrepo_commit(root_data) != EXIT_SUCCESS) {
        cli_print(cli, " ERROR: failed to commit changes!");
        return CLI_ERROR;
    }
    cli_print(cli, " changes applied successfully!");
    return CLI_OK;
}


int default_commands_init(struct cli_def *cli) {
    printf("INFO:commands.c: initializing commands\n");


    cli_set_auth_callback(cli, check_auth);

    cli_register_command(cli, NULL, NULL,
                         "exit", cmd_exit2, PRIVILEGE_UNPRIVILEGED,
                         MODE_ANY, NULL, "exit to prev mode");

    cli_register_command(cli, NULL, NULL,
                         "frr", cmd_frr, PRIVILEGE_UNPRIVILEGED,
                         MODE_CONFIG, NULL, "frr subsystem config");

    cli_register_command(cli, NULL, NULL,
                         "commit", cmd_commit, PRIVILEGE_UNPRIVILEGED,
                         MODE_ANY, NULL, "commit changes to sysrepo cdb");

    struct cli_command *print = cli_register_command(cli, NULL, NULL,
                                                     "print", NULL, PRIVILEGE_UNPRIVILEGED,
                                                     MODE_ANY, NULL, "print the candidate/running config");

    cli_register_command(cli, print, NULL,
                         "local-candidate-config", cmd_print_local_config, PRIVILEGE_UNPRIVILEGED,
                         MODE_ANY, NULL, "print the local new config data tree");
    cli_register_command(cli, print, NULL,
                         "cdb-candidate-config", NULL, PRIVILEGE_UNPRIVILEGED,
                         MODE_ANY, NULL, "print cdp candidate config");

    cli_register_command(cli, print, NULL,
                         "cdb-running-config", NULL, PRIVILEGE_UNPRIVILEGED,
                         MODE_ANY, NULL, "print cdp running config");

    cli_register_command(cli, NULL, NULL,
                         "discard-changes", cmd_discard_changes, PRIVILEGE_UNPRIVILEGED,
                         MODE_ANY, NULL, "discard all current changes");


}
