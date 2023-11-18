//
// Created by ali on 11/17/23.
//
#include "data_factory.h"

extern struct lyd_node *root_data, *parent_data;


char *create_list_path_predicate(struct lysc_node *y_node, char *argv[], int argc, int with_module_name) {
    const struct lysc_node *child_list = lysc_node_child(y_node);
    const struct lysc_node *child;
    int arg_pos = -1;
    size_t total_len = 0;
    // Calculate the total length needed for the string
    if (with_module_name)
        total_len = strlen(y_node->module->name) + strlen(y_node->name) + 2; // +2 for ":", "]", and null terminator

    LY_LIST_FOR(child_list, child) {
        if (lysc_is_key(child)) {
            arg_pos++;
            total_len += strlen(child->name) + 6 +
                         strlen(argv[arg_pos]);// +7 for "[='']", separator characters, and the length of argv

        }
    }

    // Allocate memory for the string
    char *cmd_string = (char *) malloc(total_len);
    if (!cmd_string) {
        perror("Memory allocation failed");
        return NULL;
    }

    // Construct the string
    if (with_module_name)
        sprintf(cmd_string, "%s:%s", y_node->module->name, y_node->name);
    arg_pos = -1;
    LY_LIST_FOR(child_list, child) {
        if (lysc_is_key(child)) {
            arg_pos++;
            strcat(cmd_string, "[");
            strcat(cmd_string, child->name);
            strcat(cmd_string, "=");
            strcat(cmd_string, "'");
            strcat(cmd_string, argv[arg_pos]);
            strcat(cmd_string, "'");
            strcat(cmd_string, "]");
        }
    }


    return cmd_string;
}


int add_data_node_list(struct lysc_node *y_node, struct cli_command *c, char *argv[], int argc) {
    int ret;
    char xpath[265];
    if (parent_data == NULL) {
        lysc_path(y_node, LYSC_PATH_DATA, xpath, 256);
        strcat(xpath, create_list_path_predicate(y_node, argv, argc, 0));
        ret = lyd_new_path(root_data, y_node->module->ctx, xpath, NULL, 0, &parent_data);
    } else {
        ret = lyd_new_path(parent_data, y_node->module->ctx, create_list_path_predicate(y_node, argv, argc, 1), NULL, 0,
                           &parent_data);
    }
    return ret;
}


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
            // if node is list we need to add predicate [%s='%s']
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
        case LYS_LEAFLIST:
            snprintf(xpath, 256, "%s:%s", y_node->module->name, y_node->name);
            ret = lyd_new_path2(parent_data, y_node->module->ctx, xpath,
                                value, 0, 0, LYD_NEW_PATH_UPDATE, NULL,
                                NULL);
            break;


    }
    return ret;

}