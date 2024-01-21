//
// Created by ali on 10/19/23.
//


#include "y_utils.h"
#include "yang_core.h"
#include "data_factory.h"
#include "src/onm_logger.h"


int cmd_yang_container(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    if (!is_root_node(y_node)) {
        if (argc >= 1)
            cli_print(cli, " invalid command, use '?' for valid options");
        else
            cli_print(cli, " incomplete command, please use '?' for options ");
        return CLI_ERROR;
    }

    int ret;
    int is_delete = 0;

    if (argc==1){
        if (!strcmp(argv[0],"?")){
            cli_print(cli," <cr>");
            return CLI_OK;
        }

    }

    if (argc >= 1) {
        cli_print(cli, "ERROR: unknown argument(s)");
        return CLI_ERROR_ARG;
    }

    struct cli_optarg_pair *optargs = cli->found_optargs;

    while (optargs != NULL) {
        if (strcmp(optargs->name, "delete") == 0)
            is_delete = 1;
        optargs = optargs->next;
    }

    if (is_delete) {
        ret = delete_data_node(y_node, NULL, cli);
        if (ret != LY_SUCCESS) {
            LOG_ERROR("Failed to delete the data tree");
            cli_error(cli, "failed to execute command, error with deleting the data node.");
            return CLI_ERROR;
        } else
            return CLI_OK;
    }

    // this is add operation.
    ret = add_data_node(y_node, NULL, cli);
    if (ret != LY_SUCCESS) {
        LOG_ERROR("Failed to create the data tree");
        cli_error(cli, "failed to execute command, error with adding the data node.");
        return CLI_ERROR;
    }

    // check if this is the root container which is the only container that changes config mode.
    if (y_node->parent == NULL) {
        int mode = y_get_next_mode(y_node);
        cli_push_configmode(cli, mode, (char *) cmd);
    }

    return CLI_OK;
}

int cmd_yang_no_container(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    if (argc >= 1) {
        cli_print(cli, "ERROR: unknown argument(s)");
        return CLI_ERROR_ARG;
    }
    int ret = delete_data_node(y_node, NULL, cli);
    if (ret != LY_SUCCESS) {
        LOG_ERROR("Failed to delete the data tree");
        cli_error(cli, "failed to execute command, error with deleting the data node.");
        return CLI_ERROR;
    }
    return CLI_OK;
}

enum {
    RUNNING_DS,
    STARTUP_DS,
    CANDIDATE_DS,
    OPERATIONAL_DS,
};

int core_cmd_yang_show_config(struct cli_def *cli, struct cli_command *c, int datastore) {

    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;

    char xpath[1028] = {0};
    lysc_path(y_node, LYSC_PATH_DATA, xpath, 1028);
    struct lyd_node *d_node = NULL;

    switch (datastore) {
        case CANDIDATE_DS:
            d_node = get_local_node_data(xpath);
            break;
        case RUNNING_DS:
            d_node = get_sysrepo_running_node(xpath);
            break;
        case STARTUP_DS:
            d_node = get_sysrepo_startup_node(xpath);
            break;
        case OPERATIONAL_DS:
            d_node = get_sysrepo_operational_node(xpath);
            break;
    }

    if (d_node)
        config_print(cli, d_node);
    else
        cli_print(cli, "no data found");
    return CLI_OK;
}

int cmd_yang_show_candidate_config_container(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[],
                                             int argc) {
    if (argc >= 1) {
        cli_print(cli, "ERROR: unknown argument(s)");
        return CLI_ERROR_ARG;
    }
    return core_cmd_yang_show_config(cli, c, CANDIDATE_DS);

}

int cmd_yang_show_running_config_container(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[],
                                           int argc) {
    if (argc >= 1) {
        cli_print(cli, "ERROR: unknown argument(s)");
        return CLI_ERROR_ARG;
    }
    return core_cmd_yang_show_config(cli, c, RUNNING_DS);

}

int cmd_yang_show_startup_config_container(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[],
                                           int argc) {
    if (argc >= 1) {
        cli_print(cli, "ERROR: unknown argument(s)");
        return CLI_ERROR_ARG;
    }
    return core_cmd_yang_show_config(cli, c, STARTUP_DS);

}

int
cmd_yang_show_candidate_config_diff_container(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[],
                                              int argc) {
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;
    if (argc >= 1) {
        cli_print(cli, "ERROR: unknown argument(s)");
        return CLI_ERROR_ARG;
    }
    char xpath[1028] = {0};
    lysc_path(y_node, LYSC_PATH_DATA, xpath, 1028);
    struct lyd_node *d_node = NULL;
    struct lyd_node *candidate_node = get_local_node_data(xpath);
    if (candidate_node == NULL) {
        cli_print(cli, " no config diff between candidate and running!");
        return CLI_OK;
    }
    struct lyd_node *running_node = get_sysrepo_running_node(xpath);
    lyd_diff_tree(running_node, candidate_node, 0, &d_node);

    if (d_node)
        config_print(cli, d_node);
    else
        cli_print(cli, " no config diff between candidate and running!");

    return CLI_OK;
}

int cmd_yang_show_operational_container(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[],
                                           int argc) {
    if (argc >= 1) {
        cli_print(cli, "ERROR: unknown argument(s)");
        return CLI_ERROR_ARG;
    }
    return core_cmd_yang_show_config(cli, c, OPERATIONAL_DS);
}

int has_list_in_parents(struct lysc_node *y_node){
    struct lysc_node *curr_node= y_node->parent;
    while (curr_node != NULL){
        if (curr_node->nodetype == LYS_LIST)
            return 1;
        curr_node = curr_node->parent;
    }
    return 0;

}

int register_cmd_container(struct cli_def *cli, struct lysc_node *y_node) {
    const struct lys_module *y_root_module = lysc_owner_module(y_node);
    char *cmd_hash = (char *) y_root_module->name;
    // show operational support root container only.
    if (has_oper_children(y_node) && y_node->parent == NULL) {
        char show_oper_help[100] = {0};
        sprintf(show_oper_help, "show operational data for %s (%s) [contain]", y_node->name, y_node->module->name);
        struct cli_command *show_oper_c_parent;
        show_oper_c_parent = ((struct cli_ctx_data *) cli_get_context(cli))->show_operational_data;

        cli_register_command(cli, show_oper_c_parent, y_node,
                             y_node->name,
                             cmd_yang_show_operational_container,
                             PRIVILEGE_PRIVILEGED,
                             MODE_ANY, cmd_hash, show_oper_help);

    }
    if (y_node->flags & LYS_CONFIG_R)
        return CLI_OK;

    char help[100], no_help[100], show_help[100];
    sprintf(help, "configure %s (%s) [contain]", y_node->name, y_node->module->name);
    sprintf(no_help, "delete %s (%s) [contain]", y_node->name, y_node->module->name);
    sprintf(show_help, "show %s configurations (%s)", y_node->name, y_node->module->name);

    unsigned int mode;

    // there is ietf-yang where container and choice has same name.
    // we don't want to register this container to avoid duplication.
    if (y_node->parent != NULL && !strcmp(y_node->parent->name, y_node->name))
        return 1;
    // check if parent is container or choice and is not the root module ,if yes attach the command to the container command.
    struct cli_command *parent_cmd = find_parent_cmd(cli, y_node);
    struct cli_command *parent_cmd_no = find_parent_no_cmd(cli, y_node);

    // show config currently support root container only
    if (!has_list_in_parents(y_node)) {
        struct cli_command *parent_cmd_show_conf_cand = find_parent_show_candidate_cmd(cli, y_node);
        struct cli_command *parent_cmd_show_conf_run = find_parent_show_running_cmd(cli, y_node);
        struct cli_command *parent_cmd_show_conf_start = find_parent_show_startup_cmd(cli, y_node);
        if (parent_cmd_show_conf_cand == NULL) {
            parent_cmd_show_conf_cand = ((struct cli_ctx_data *) cli_get_context(cli))->show_conf_cand_cmd;
            parent_cmd_show_conf_run = ((struct cli_ctx_data *) cli_get_context(cli))->show_conf_running_cmd;
            parent_cmd_show_conf_start = ((struct cli_ctx_data *) cli_get_context(cli))->show_conf_startup_cmd;
        }
        struct cli_command *show_cand_cont_c = cli_register_command(cli, parent_cmd_show_conf_cand, y_node,
                                                                    y_node->name,
                                                                    cmd_yang_show_candidate_config_container,
                                                                    PRIVILEGE_PRIVILEGED,
                                                                    MODE_ANY, cmd_hash, show_help);
        cli_register_command(cli, parent_cmd_show_conf_run, y_node,
                             y_node->name,
                             cmd_yang_show_running_config_container,
                             PRIVILEGE_PRIVILEGED,
                             MODE_ANY, cmd_hash, show_help);
        cli_register_command(cli, parent_cmd_show_conf_start, y_node,
                             y_node->name,
                             cmd_yang_show_startup_config_container,
                             PRIVILEGE_PRIVILEGED,
                             MODE_ANY, cmd_hash, show_help);
        cli_register_command(cli, show_cand_cont_c, y_node,
                             "diff",
                             cmd_yang_show_candidate_config_diff_container,
                             PRIVILEGE_PRIVILEGED,
                             MODE_ANY, "diff", "show config diff");

    }

    if (parent_cmd == NULL)
        mode = y_get_curr_mode(y_node);
    else
        mode = parent_cmd->mode;

    if (parent_cmd_no == NULL)
        parent_cmd_no = ((struct cli_ctx_data *) cli_get_context(cli))->no_cmd;


    cli_register_command(cli, parent_cmd, y_node, y_node->name,
                         cmd_yang_container, PRIVILEGE_PRIVILEGED,
                         mode, cmd_hash, help);

    cli_register_command(cli, parent_cmd_no, y_node, y_node->name,
                         cmd_yang_no_container, PRIVILEGE_PRIVILEGED,
                         mode, cmd_hash, no_help);


    return EXIT_SUCCESS;
}