//
// Created by ali on 4/25/24.
//

#ifndef XAIN_DATA_PRINT_H
#define XAIN_DATA_PRINT_H
#include <libyang/libyang.h>
#include <libyang/printer_data.h>
#include "lib/libcli/libcli.h"

void config_print_mem(char **result, struct lyd_node *d_node);
#endif //XAIN_DATA_PRINT_H
