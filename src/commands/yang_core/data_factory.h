//
// Created by ali on 11/17/23.
//

#ifndef ONMCLI_DATA_FACTORY_H
#define ONMCLI_DATA_FACTORY_H
#include <libyang/libyang.h>
#include <libyang/parser_data.h>
#include <libyang/tree_data.h>
#include <libyang/printer_data.h>
#include <libyang/tree.h>
#include <libyang/log.h>
#include "lib/libcli/libcli.h"
int add_data_node(struct lysc_node *y_node, struct cli_command *c, char *value);
#endif //ONMCLI_DATA_FACTORY_H
