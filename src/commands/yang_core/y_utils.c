//
// Created by ali on 10/21/23.
//
#include "y_utils.h"

#define CONFIG_MODE 1

int y_get_curr_mode(struct lysc_node *y_node) {
    unsigned int mode;
    const struct lys_module *y_root_module = lysc_owner_module(y_node);
    if (y_node->parent == NULL)
        mode = CONFIG_MODE;
    else{
        char xpath[256];
        lysc_path(y_node->parent, LYSC_PATH_DATA, xpath, 256);
        mode = str2int_hash(xpath, NULL);
    }

    return mode;
}

int y_get_next_mode(struct lysc_node *y_node) {
    char xpath[256];
    lysc_path(y_node, LYSC_PATH_DATA, xpath, 256);
    unsigned int mode = str2int_hash(xpath, NULL);
    return mode;
}