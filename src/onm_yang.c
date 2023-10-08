//
// Created by ali on 9/29/23.
//

#include <stdio.h>
#include <unistd.h>
#include "utils.h"
#include "onm_yang.h"
#include "config.h"


static struct ly_ctx *ctx;



// Libyang utils functions

int set_yang_searchdir(const char *dir) {
    printf("INFO:onm_yang.c: setting yang search path to `%s`\n",dir);
    ly_ctx_set_searchdir(ctx, dir);
    return 0;
}
int unset_yang_searchdir(const char *dir) {
    printf("INFO:onm_yang.c: setting yang search path to `%s`\n",dir);
    ly_ctx_unset_searchdir(ctx, dir);
    return 0;
}

const char * const* get_yang_searchdirs(){
    return ly_ctx_get_searchdirs(ctx);
}

const struct lys_module *get_module_schema(char *module_name) {
    printf("DEBUG:onm_yang.c: get schema for module=%s\n",module_name);

    const struct lys_module *module = ly_ctx_load_module(ctx, module_name, NULL, NULL);
//    const struct lys_module *module1 = ly_ctx_load_module(ctx, "ietf-interfaces", NULL, NULL);
    return module;


}

int onm_yang_init() {
    printf("INFO:onm_yang.c:initializing onm yang context\n");
    int ret;
    if (LIBYANG_LOG_DEBUG == 1)
        ly_log_level(LY_LLDBG);
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("yang:getcwd:");
        return -1;
    }

    ret = ly_ctx_new("yang/standard/ietf/RFC", LY_CTX_ALL_IMPLEMENTED, &ctx);
    if (ret > 0) {
        fprintf(stderr, "Failed to create libyang context: %d\n", ret);
        return -1;
    }
    return 0;
}