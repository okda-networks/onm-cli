/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef ONMCLI_YANG_CMD_GENERATOR_H
#define ONMCLI_YANG_CMD_GENERATOR_H

#include <libyang/libyang.h>
#include <libyang/parser_data.h>
#include <libyang/log.h>
#include "lib/libcli/libcli.h"

int sysrepo_commands_init(struct cli_def *cli);

#endif //ONMCLI_YANG_CMD_GENERATOR_H
