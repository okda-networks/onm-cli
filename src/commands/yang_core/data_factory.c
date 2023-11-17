//
// Created by ali on 11/17/23.
//
#include "data_factory.h"

extern struct lyd_node *root_data, *parent_data;

int add_data_node(struct lysc_node *y_node, struct cli_command *c, char *value) {
    int ret;
    char xpath[256], xpath_list_etx[128];

    switch (y_node->nodetype) {
        case LYS_CONTAINER:
            // for container, we need to check, if the parent is null, then this is the first child of the root
            // if it's not then add the container to the current parent.
            if (parent_data == NULL) {
                lysc_path(y_node, LYSC_PATH_DATA, xpath, 256);
                ret = lyd_new_path2(NULL, y_node->module->ctx,
                                    xpath, NULL, 0, 0,
                                    0, &root_data, &parent_data);
            } else {
                snprintf(xpath, 256, "%s:%s", y_node->module->name, y_node->name);
                ret = lyd_new_path(parent_data, y_node->module->ctx, xpath,
                                   NULL, LYD_NEW_PATH_UPDATE, &parent_data);
            }
            break;
        case LYS_LIST:
            if (parent_data == NULL) {
                lysc_path(y_node, LYSC_PATH_DATA, xpath, 256);
                sprintf(xpath_list_etx, "[%s='%s']", c->optargs->name, value);
                strcat(xpath, xpath_list_etx);
                ret = lyd_new_path(root_data, y_node->module->ctx, xpath, NULL, 0, &parent_data);
            } else {
                snprintf(xpath, 256, "%s:%s[%s='%s']", y_node->module->name, y_node->name, c->optargs->name, value);
                ret = lyd_new_path(parent_data, y_node->module->ctx, xpath, NULL, 0, &parent_data);
            }
            break;

        case LYS_LEAF:
            snprintf(xpath, 256, "%s:%s", y_node->module->name, y_node->name);

            ret = lyd_new_path2(parent_data, y_node->module->ctx, xpath,
                                value, 0, 0, LYD_NEW_PATH_UPDATE, NULL,
                                NULL);
            break;


    }
    return ret;

}