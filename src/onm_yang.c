//
// Created by ali on 9/29/23.
//

#include <stdio.h>
#include <unistd.h>
#include "utils.h"
#include "onm_yang.h"
#include "config.h"


struct ly_ctx *yang_ctx;
struct lyd_node *root_data,*parent_data;

static void print_ly_err(const struct ly_err_item *err) {
    while (err) {
        fprintf(stderr, "libyang error: %s\n", err->msg);
        err = err->next;
    }
}


const struct lys_module *get_module_schema(char *module_name) {
    printf("DEBUG:onm_yang.c: get schema for module=%s\n", module_name);
    const struct lys_module *module = ly_ctx_load_module(yang_ctx, module_name, NULL, NULL);
    return module;
}

struct ly_ctx *get_yang_context() {
    return yang_ctx;
}

int onm_yang_init() {
    printf("INFO:onm_yang.c:initializing onm yang context\n");
    int ret;
    if (LIBYANG_LOG_DEBUG == 1)
        ly_log_level(LY_LLDBG);
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("yang:getcwd:");
        return EXIT_FAILURE;
    }

    ret = ly_ctx_new("yang/standard/ietf/RFC", LY_CTX_LEAFREF_EXTENDED|LY_CTX_EXPLICIT_COMPILE, &yang_ctx);
    if (ret > 0) {
        fprintf(stderr, "Failed to create libyang context: %d\n", ret);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}