//
// Created by ali on 10/3/23.
//

#ifndef ONMCLI_ONM_YANG_H
#define ONMCLI_ONM_YANG_H

#include <libyang/libyang.h>
#include <libyang/parser_data.h>
#include <libyang/tree.h>
#include <libyang/tree_schema.h>
#include <libyang/log.h>

enum {
    ERR_GET_MODULE_NOT_FOUND = 1,
    ERR_GET_MODULE_AUG_MODULE,
    ERR_GET_MODULE_NOT_COMPILED,
    ERR_GET_MODULE_UNKNOWN
};

int onm_yang_init();

int set_yang_searchdir(const char *dir);

int unset_yang_searchdir(const char *dir);

const char *const *get_yang_searchdirs();

const struct lys_module *get_module_schema(char *module_name);
struct ly_ctx* get_yang_context();


#endif //ONMCLI_ONM_YANG_H
