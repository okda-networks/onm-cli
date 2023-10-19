//
// Created by ali on 10/19/23.
//

#ifndef ONMCLI_YANG_CORE_H
#define ONMCLI_YANG_CORE_H

#include <libyang/libyang.h>
#include <libyang/parser_data.h>
#include <libyang/log.h>
#include "../../../lib/libcli/libcli.h"
#include "../../onm_yang.h"


int register_cmd_leaf_list(struct cli_def *cli, struct lysc_node *y_node);

int register_cmd_leaf(struct cli_def *cli, struct lysc_node *y_node);

int register_cmd_list(struct cli_def *cli, struct lysc_node *y_node);

int register_cmd_container(struct cli_def *cli, struct lysc_node *y_node);

int cmd_yang_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);

int cmd_yang_container(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);

int cmd_yang_leaf_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);

int cmd_yang_leaf(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);


#endif //ONMCLI_YANG_CORE_H
