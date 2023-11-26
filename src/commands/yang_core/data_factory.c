//
// Created by ali on 11/17/23.
//
#include "data_factory.h"
#include "src/onm_sysrepo.h"
#include "y_utils.h"


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
    char *predicate_str = (char *) malloc(total_len);
    memset(predicate_str, '\0', total_len);
    if (!predicate_str) {
        perror("Memory allocation failed");
        return NULL;
    }

    // Construct the string
    if (with_module_name)
        sprintf(predicate_str, "%s:%s", y_node->module->name, y_node->name);
    arg_pos = -1;
    LY_LIST_FOR(child_list, child) {
        if (lysc_is_key(child)) {
            arg_pos++;
            strcat(predicate_str, "[");
            strcat(predicate_str, child->name);
            strcat(predicate_str, "=");
            strcat(predicate_str, "'");
            strcat(predicate_str, argv[arg_pos]);
            strcat(predicate_str, "'");
            strcat(predicate_str, "]");
        }
    }


    return predicate_str;
}


int add_data_node_list(struct lysc_node *y_node, struct cli_command *c, char *argv[], int argc) {
    int ret;
    char xpath[265];
    char *predicate_str;
    memset(xpath, '\0', 256);
    struct ly_ctx *sysrepo_ctx = (struct ly_ctx *) sysrepo_get_ctx();
    if (!sysrepo_ctx) {
        printf(" add_data_node(): Failure: failed to get sysrepo_ctx");
        return EXIT_FAILURE;
    }
    struct lyd_node *curr_parent, *new_parent;
    // set current parent and xpath based on the list location in the tree.
    if (parent_data == NULL) {
        curr_parent = root_data;
        lysc_path(y_node, LYSC_PATH_DATA, xpath, 256);
        strcat(xpath, create_list_path_predicate(y_node, argv, argc, 0));
    } else {
        curr_parent = parent_data;
        strcat(xpath, create_list_path_predicate(y_node, argv, argc, 1));
    }
    ret = lyd_find_path(curr_parent, xpath, 0, &new_parent);
    if (new_parent == NULL)
        ret = lyd_new_path(curr_parent, sysrepo_ctx, xpath, NULL, LYD_NEW_PATH_UPDATE, &parent_data);
    else
        parent_data = new_parent;

    free(predicate_str);
    if (ret != LY_SUCCESS) {
        print_ly_err(ly_err_first(sysrepo_ctx));
    }
    return ret;
}

// edit type
enum {
    EDIT_DATA_ADD,
    EDIT_DATA_DEL,
};

static int edit_node_data_tree(struct lysc_node *y_node, char *value, int edit_type, struct lyd_node **out_node) {
    int ret;
    char xpath[256];
    memset(xpath, '\0', 256);
    struct ly_ctx *sysrepo_ctx = (struct ly_ctx *) sysrepo_get_ctx();
    if (!sysrepo_ctx) {
        printf(" add_data_node(): Failure: failed to get sysrepo_ctx");
        return EXIT_FAILURE;
    }
    switch (y_node->nodetype) {
        case LYS_CONTAINER: {
            struct lyd_node *new_parent = NULL;
            if (parent_data == NULL) {
                lysc_path(y_node, LYSC_PATH_DATA, xpath, 256);
            } else {
                snprintf(xpath, 256, "%s:%s", y_node->module->name, y_node->name);
            }

            // check if the node exist in the tree, if not create new node in the tree.
            ret = lyd_find_path(parent_data, xpath, 0, &new_parent);
            if (new_parent == NULL) {
                ret = lyd_new_path(parent_data, sysrepo_ctx, xpath, NULL, LYD_NEW_PATH_UPDATE, &new_parent);
            }

            // if the edit is add operation then update the parent_node, else which is delete then just set the out node.
            if (edit_type == EDIT_DATA_ADD)
                parent_data = new_parent;
            else
                *out_node = new_parent;

            root_data = root_data ? root_data : parent_data;
        }

            break;
        case LYS_LEAF:
        case LYS_LEAFLIST:
            snprintf(xpath, 256, "%s:%s", y_node->module->name, y_node->name);
            ret = lyd_new_path2(parent_data, sysrepo_ctx, xpath,
                                value, 0, 0, LYD_NEW_PATH_UPDATE, NULL,
                                out_node);
            break;


    }
    if (ret != LY_SUCCESS)
        print_ly_err(ly_err_first(sysrepo_ctx));


    return ret;
}

int add_data_node(struct lysc_node *y_node, char *value) {
    // for add node we just create the node in the data tree, we don't need the lyd_node.
    return edit_node_data_tree(y_node, value, EDIT_DATA_ADD, NULL);

}

void get_xpath(struct lysc_node *y_node, char xpath[]) {
    // get the parent path
    lyd_path(parent_data, LYD_PATH_STD, xpath, 1024);
    strcat(xpath, "/");
    strcat(xpath, y_node->module->name);
    strcat(xpath, ":");
    strcat(xpath, y_node->name);

}

int delete_data_node(struct lysc_node *y_node, char *value) {
    int ret;
    struct lyd_node *n;
    ret = edit_node_data_tree(y_node, value, EDIT_DATA_DEL, &n);
    if (ret != LY_SUCCESS)
        return ret;

    char xpath[1024];
    memset(xpath, '\0', 1024);

    lyd_path(n, LYD_PATH_STD, xpath, 1024);
    lyd_free_tree(n);

    sr_session_ctx_t *session = sysrepo_get_session();
    if (session == NULL) {
        printf("delete_data_node: failed to get sr_session\n");
        return EXIT_FAILURE;
    }

    ret = sr_delete_item(session, xpath, 0);
    if (ret != SR_ERR_OK) {
        printf("delete_data_node: sr_delete_item failed\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;

}