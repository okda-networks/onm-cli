//
// Created by ali on 10/4/23.
//

#ifndef ONMCLI_COMMANDS_H
#define ONMCLI_COMMANDS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <libyang/libyang.h>
#include <libyang/parser_data.h>
#include <libyang/log.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../lib/libcli/libcli.h"
#include "../config.h"



int onm_commands_init(struct cli_def *cli);

#endif //ONMCLI_COMMANDS_H
