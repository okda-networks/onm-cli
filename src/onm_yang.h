//
// Created by ali on 10/3/23.
//

#ifndef ONMCLI_ONM_YANG_H
#define ONMCLI_ONM_YANG_H

#include <libyang/libyang.h>
#include <libyang/parser_data.h>
#include <libyang/log.h>

int onm_yang_init();

int set_yang_searchdir(const char *dir);
int unset_yang_searchdir(const char *dir);
const char * const* get_yang_searchdirs();

struct lysc_node *get_module_schema(char *module_name);

#endif //ONMCLI_ONM_YANG_H
