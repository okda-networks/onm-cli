//
// Created by ali on 10/21/23.
//

#ifndef ONMCLI_Y_UTILS_H
#define ONMCLI_Y_UTILS_H
#include "../../utils.h"
#include <libyang/tree_schema.h>
int y_get_curr_mode(struct lysc_node * y_node);
int y_get_next_mode(struct lysc_node * y_node);
#endif //ONMCLI_Y_UTILS_H
