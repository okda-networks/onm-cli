//
// Created by ali on 10/4/23.
//

#include "src/utils.h"
#include "default_cmd.h"
#include "src/onm_sysrepo.h"
#include "yang_core/data_factory.h"
#include "src/onm_logger.h"


#ifdef __GNUC__
#define UNUSED(d) d __attribute__((unused))
#else
#define UNUSED(d) d
#endif

extern struct lyd_node *parent_data;


unsigned int regular_count = 0;
unsigned int debug_regular = 0;


int cmd_regular_callback(struct cli_def *cli) {
    regular_count++;
    if (debug_regular) {
        cli_print(cli, "Regular callback - %u times so far", regular_count);
        cli_reprompt(cli);
    }
    return CLI_OK;
}

int cmd_no(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    cli_print(cli, "incomplete command!");
    return CLI_ERROR;
}

int cmd_discard_changes(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    struct data_tree *config_dtree = get_config_root_tree();

    // commit changes.
    if (config_dtree == NULL) {
        cli_print(cli, " no config changes found!");
        return CLI_OK;
    }
    free_data_tree_all();

    if (sysrepo_discard_changes() != SR_ERR_OK) {
        cli_print(cli, "failed to discard changes, sysrepo error!");
        return CLI_ERROR;
    }
    cli_print(cli, "config changes discarded!");
    cli_set_configmode(cli, MODE_CONFIG, NULL);
    return CLI_OK;
}

int cmd_exit2(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    struct data_tree *config_dtree = get_config_root_tree();
    if (cli->mode == MODE_CONFIG) {
        if (config_dtree != NULL) {
            struct data_tree *curr_root = config_dtree;
            while (curr_root != NULL) {
                // 1 indicate there is config diff between sysrepo and local candidate
                if (sysrepo_has_uncommited_changes(curr_root->node) == 1) {
                    cli_print(cli,
                              "ERROR: there are uncommitted changes, please `commit` or `discard-changes` before exist!");
                    return CLI_ERROR;
                }
                curr_root = curr_root->prev;
            }
            free_data_tree_all();
        }
        sysrepo_release_ctx();
        return cli_exit(cli, c, cmd, argv, argc);
    }
    // we need to shift the parent_data backward with each exit call.
    if (parent_data != NULL) {
        struct lyd_node *prev_parent = (struct lyd_node *) parent_data->parent;
        while (prev_parent != NULL) {
            if (prev_parent->schema->nodetype == LYS_LIST || is_root_node(prev_parent->schema))
                break;
            prev_parent = (struct lyd_node *) prev_parent->parent;
        }
        parent_data = (struct lyd_node *) prev_parent;
    }

    return cli_exit(cli, c, cmd, argv, argc);
}

int cmd_show_config_running(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    cli_print(cli, "not implemented, use show config-running <node>");
    return CLI_OK;
}

int cmd_show_config_startup(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    cli_print(cli, "not implemented, use show config-startup <node>");
    return CLI_OK;
}


int cmd_show_config_candidate(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    struct data_tree *config_dtree = get_config_root_tree();
    int is_xml = 1;
    char *format = cli_get_optarg_value(cli, "format", NULL);
    if (format != NULL) {
        to_lower(format);
        if (strcmp(format, "json") == 0)
            is_xml = 0;
    }
    // commit changes.
    if (config_dtree == NULL) {
        cli_print(cli, "no new config yet!");
        return CLI_ERROR;
    }
    struct data_tree *curr_root = config_dtree;
    while (curr_root != NULL) {
        char *result;
        if (is_xml == 0)
            lyd_print_mem(&result, curr_root->node, LYD_JSON, 0);
        else
            lyd_print_mem(&result, curr_root->node, LYD_XML, 0);
        cli_print(cli, result, NULL);
        curr_root = curr_root->prev;
    }

    return CLI_OK;
}

int cmd_commit(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    struct data_tree *config_dtree = get_config_root_tree();
    // commit changes.
    if (config_dtree == NULL) {
        cli_print(cli, " no modification to commit!");
        return CLI_OK;
    }
    int change_pushed = 0;
    struct data_tree *curr_root = config_dtree;
    while (curr_root != NULL) {
        if (sysrepo_has_uncommited_changes(curr_root->node) == 0)
            goto next;;
        if (sysrepo_commit(curr_root->node) != EXIT_SUCCESS) {
            cli_print(cli, "commit_failed: failed to commit changes!");
            return CLI_ERROR;
        } else {
            change_pushed = 1;
        }
        next:
        curr_root = curr_root->prev;
    }
    if (change_pushed)
        cli_print(cli, " changes applied successfully!");
    else
        cli_print(cli, " no modification to commit!");
    return CLI_OK;
}


int default_commands_init(struct cli_def *cli) {
    LOG_INFO("commands.c: initializing commands\n");

    cli_register_command(cli, NULL, NULL,
                         "exit", cmd_exit2, PRIVILEGE_UNPRIVILEGED,
                         MODE_ANY, NULL, "exit to prev mode");

    cli_register_command(cli, NULL, NULL,
                         "commit", cmd_commit, PRIVILEGE_UNPRIVILEGED,
                         MODE_ANY, NULL, "commit changes to sysrepo cdb");


    struct cli_command *show = cli_register_command(cli, NULL, NULL,
                                                    "show", NULL, PRIVILEGE_UNPRIVILEGED,
                                                    MODE_ANY, NULL, "print the candidate/running config");

//    struct cli_command *config_running = cli_register_command(cli, show, NULL,
//                                                              "config-running", NULL, PRIVILEGE_UNPRIVILEGED,
//                                                              MODE_ANY, NULL, "print the candidate/running config");

    struct cli_command *config_candidate = cli_register_command(cli, show, NULL,
                                                                "config-candidate", cmd_show_config_candidate,
                                                                PRIVILEGE_UNPRIVILEGED,
                                                                MODE_ANY, "config-candidate",
                                                                "show the candidate configurations");
    struct cli_command *config_running = cli_register_command(cli, show, NULL,
                                                              "config-running", cmd_show_config_running,
                                                              PRIVILEGE_UNPRIVILEGED,
                                                              MODE_ANY, "config-running",
                                                              "show the running configurations");
    struct cli_command *config_startup = cli_register_command(cli, show, NULL,
                                                              "config-startup", cmd_show_config_startup,
                                                              PRIVILEGE_UNPRIVILEGED,
                                                              MODE_ANY, NULL, "show the startup configurations");

//    cli_register_optarg(config_running, "format", CLI_CMD_OPTIONAL_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_ANY,
//                        "printed format [json|xml].", NULL, NULL, NULL);

    cli_register_optarg(config_candidate, "format", CLI_CMD_OPTIONAL_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_ANY,
                        "printed format [json|xml].", NULL, NULL, NULL);


    cli_register_command(cli, NULL, NULL,
                         "discard-changes", cmd_discard_changes, PRIVILEGE_UNPRIVILEGED,
                         MODE_ANY, NULL, "discard all current changes");
    struct cli_command *no_cmd = cli_register_command(cli, NULL, NULL,
                                                      "no", cmd_no, PRIVILEGE_UNPRIVILEGED,
                                                      MODE_ANY, NULL, "delete configs");

    struct cli_ctx_data *ctx_data = (struct cli_ctx_data *) cli_get_context(cli);
    ctx_data->no_cmd = no_cmd;
    ctx_data->show_conf_cand_cmd = config_candidate;
    ctx_data->show_conf_running_cmd = config_running;
    ctx_data->show_conf_startup_cmd = config_startup;

    return EXIT_SUCCESS;

}
