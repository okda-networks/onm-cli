//
// Created by ali on 10/21/23.
//

#ifndef ONMCLI_Y_UTILS_H
#define ONMCLI_Y_UTILS_H

#include "src/utils.h"
#include <libyang/tree_schema.h>
#include <libyang/tree_data.h>
#include <libyang/printer_data.h>

struct cli_ctx_data {
    struct cli_command *no_cmd;
    struct cli_command *show_conf_cand_cmd;
    struct cli_command *show_conf_running_cmd;
    struct cli_command *show_conf_startup_cmd;
    struct cli_command *show_operational_data;
    struct cli_command *print_order_cmd;
};

int is_root_node(const struct lysc_node *y_node);

int has_oper_children(struct lysc_node *y_node);

void print_ly_err(const struct ly_err_item *err, char *component, struct cli_def *cli);

void config_print(struct cli_def *cli, struct lyd_node *d_node);

int y_get_curr_mode(struct lysc_node *y_node);

int y_get_next_mode(struct lysc_node *y_node);

const char *creat_help_for_identity_type(struct lysc_node *y_node);

const char *get_relative_path(struct lysc_node *y_node);

struct cli_command *find_parent_cmd(struct cli_def *cli, struct lysc_node *y_node);

struct cli_command *find_parent_no_cmd(struct cli_def *cli, struct lysc_node *y_node);

struct cli_command *find_parent_show_candidate_cmd(struct cli_def *cli, struct lysc_node *y_node);

struct cli_command *find_parent_show_running_cmd(struct cli_def *cli, struct lysc_node *y_node);

struct cli_command *find_parent_show_startup_cmd(struct cli_def *cli, struct lysc_node *y_node);

struct cli_command *find_parent_show_oper_cmd(struct cli_def *cli, struct lysc_node *y_node);

#endif //ONMCLI_Y_UTILS_H
