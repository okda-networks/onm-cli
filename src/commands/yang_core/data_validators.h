//
// Created by ali on 10/22/23.
//

#ifndef ONMCLI_DATA_VALIDATORS_H
#define ONMCLI_DATA_VALIDATORS_H

#include "../../../lib/libcli/libcli.h"

int yang_data_validator(struct cli_def *cli, const char *word, const char *value,void * cmd_model);

#endif //ONMCLI_DATA_VALIDATORS_H
