//
// Created by ali on 10/7/23.
//

#ifndef XAIN_YANG_COMMANDS_H
#define XAIN_YANG_COMMANDS_H
#include <libyang/libyang.h>
#include <libyang/parser_data.h>
#include <libyang/log.h>
#include "../../lib/libcli/libcli.h"
#include "../onm_yang.h"

int yang_cmds_init(struct cli_def *cli);
#endif //XAIN_YANG_COMMANDS_H
