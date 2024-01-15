//
// Created by ali on 10/21/23.
//

#ifndef ONMCLI_Y_UTILS_H
#define ONMCLI_Y_UTILS_H
#include "src/utils.h"
#include <libyang/tree_schema.h>
void print_ly_err(const struct ly_err_item *err, char *component, struct cli_def *cli);
int y_get_curr_mode(struct lysc_node * y_node);
int y_get_next_mode(struct lysc_node * y_node);
const char * creat_help_for_identity_type(struct lysc_node *y_node);
struct cli_command *get_cli_yang_command(struct cli_def *cli,struct lysc_node **y_node);
const char * get_relative_path(struct lysc_node *y_node);
struct cli_command* find_parent_cmd(struct cli_def *cli,struct lysc_node * y_node);
#endif //ONMCLI_Y_UTILS_H
