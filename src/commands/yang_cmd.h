//
// Created by ali on 10/7/23.
//

#ifndef ONMCLI_YANG_COMMANDS_H
#define ONMCLI_YANG_COMMANDS_H

#include <libyang/libyang.h>
#include <libyang/parser_data.h>
#include <libyang/log.h>
#include "../../lib/libcli/libcli.h"
#include "../onm_yang.h"

int cmd_yang_container(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);

int cmd_yang_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);

int cmd_yang_leaf(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);

int cmd_yang_leaf_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);

int cmd_yang_path(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);


#endif //ONMCLI_YANG_COMMANDS_H
