//
// Created by ali on 10/22/23.
//

#ifndef ONMCLI_DATA_VALIDATORS_H
#define ONMCLI_DATA_VALIDATORS_H

#include "../../../lib/libcli/libcli.h"
#include "../../onm_yang.h"
#include <libyang/libyang.h>
#include <libyang/parser_data.h>
#include <libyang/tree.h>
#include <libyang/log.h>
#include <stdio.h>

int yang_data_validator(struct cli_def *cli, const char *word, const char *value,void * cmd_model);

#endif //ONMCLI_DATA_VALIDATORS_H
