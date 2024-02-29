/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef ONMCLI_YANG_CORE_H
#define ONMCLI_YANG_CORE_H

#include <libyang/libyang.h>
#include <libyang/parser_data.h>
#include <libyang/tree_data.h>
#include <libyang/printer_data.h>
#include <libyang/tree.h>
#include <libyang/log.h>
#include "lib/libcli/libcli.h"

char *get_model_org_prefix(char *module_name);

int register_cmd_leaf_list(struct cli_def *cli, struct lysc_node *y_node);

int register_cmd_leaf(struct cli_def *cli, struct lysc_node *y_node);

int register_cmd_list(struct cli_def *cli, struct lysc_node *y_node);

int register_cmd_container(struct cli_def *cli, struct lysc_node *y_node);

int register_cmd_choice(struct cli_def *cli, struct lysc_node *y_node);

int register_commands_schema(struct lysc_node *schema, struct cli_def *cli);

int unregister_commands_schema(struct lysc_node *schema, struct cli_def *cli);

#endif //ONMCLI_YANG_CORE_H
