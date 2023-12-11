//
// Created by ali on 11/17/23.
//
#include "data_factory.h"
#include "src/onm_sysrepo.h"
#include "y_utils.h"


extern struct lyd_node *parent_data;
struct data_tree *curr_root;
struct data_tree *config_root_tree;

struct data_tree *get_config_root_tree() {
    return config_root_tree;
}

void free_data_tree_all() {
    struct data_tree *curr_node = config_root_tree;
    while (curr_node != NULL) {
        lyd_free_all(curr_node->node);
        curr_node = curr_node->prev;
    }
    config_root_tree = NULL;

}

// edit type
enum {
    EDIT_DATA_ADD,
    EDIT_DATA_DEL,
};

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

int edit_node_data_tree_list(struct lysc_node *y_node, char *argv[], int argc, int edit_type,
                             struct lyd_node **out_node, int index) {
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
        curr_parent = curr_root->node;
        lysc_path(y_node, LYSC_PATH_DATA, xpath, 256);
        predicate_str = create_list_path_predicate(y_node, argv, argc, 0);
        strcat(xpath, predicate_str);
    } else {
        curr_parent = parent_data;
        predicate_str = create_list_path_predicate(y_node, argv, argc, 1);
        strcat(xpath, predicate_str);
    }

    ret = lyd_find_path(curr_parent, xpath, 0, &new_parent);
    if (new_parent == NULL)
        ret = lyd_new_path(curr_parent, sysrepo_ctx, xpath, NULL, LYD_NEW_PATH_UPDATE, &new_parent);

    if (index) {

        // index start from 10 and the step is 10, 10,20,30...
        int curr_indx = 10;
        struct lyd_node *next = NULL;
        struct lyd_node *orderd_nodes = lyd_first_sibling(lyd_child(parent_data));


        LY_LIST_FOR(orderd_nodes, next) {
            if (index < curr_indx) {
                ret = lyd_insert_before(next, new_parent);
                if (ret != LY_SUCCESS)
                    goto done;
                else
                    break;
            }
            curr_indx += 10;
        }
    }
    if (edit_type == EDIT_DATA_ADD)
        parent_data = new_parent;
    else
        *out_node = new_parent;

    done:
    free(predicate_str);
    if (ret != LY_SUCCESS) {
        print_ly_err(ly_err_first(sysrepo_ctx), "data_factory.c");
    }
    return ret;
}


int add_data_node_list(struct lysc_node *y_node, char *argv[], int argc, int index) {
    int ret = edit_node_data_tree_list(y_node, argv, argc, EDIT_DATA_ADD, NULL, index);
    return ret;
}

int delete_data_node_list(struct lysc_node *y_node, char *argv[], int argc) {
    int ret;
    struct lyd_node *n;
    ret = edit_node_data_tree_list(y_node, argv, argc, EDIT_DATA_DEL, &n, 0);// no index use key for delete
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

struct lyd_node *get_sysrepo_root(char *xpath) {
    sr_data_t *sysrepo_subtree;
    int ret = sr_get_subtree(sysrepo_get_session(), xpath, 0, &sysrepo_subtree);
    if (ret == SR_ERR_OK)
        return sysrepo_subtree->tree;
    if (ret == SR_ERR_NOT_FOUND)
        return NULL;

    printf("data_factory.c: error returning sysrepo data, code=%d\n", ret);
    return NULL;

}

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
        case LYS_CASE:
        case LYS_CONTAINER: {
            if (parent_data == NULL) {
                lysc_path(y_node, LYSC_PATH_DATA, xpath, 256);
            } else {
                snprintf(xpath, 256, "%s:%s", y_node->module->name, y_node->name);
            }
            // set the config_data_tree
            // check if this is first node in the schema, to set the root node.
            if (y_node->parent == NULL) {
                if (config_root_tree == NULL) {
                    config_root_tree = malloc(sizeof(struct data_tree));
                    config_root_tree->node = get_sysrepo_root(xpath);
                    config_root_tree->prev = NULL;
                    curr_root = config_root_tree;
                    if (config_root_tree->node  != NULL){
                        parent_data= config_root_tree->node;
                        return EXIT_SUCCESS;
                    }

                } else {
                    // check if data for this schema already exist in the tree. and use that tree if not allocat a new one
                    // and link it to config_data_tree
                    curr_root = config_root_tree;
                    while (curr_root != NULL) {
                        if (strcmp(curr_root->node->schema->name, y_node->name) == 0 && y_node->parent == NULL) {
                            // root data tree found, we just set parent_data to the found root and exit without creating
                            // new path.
                            parent_data = curr_root->node;
                            return LY_SUCCESS;
                        }
                        curr_root = curr_root->prev;
                    }
                    // create new root_tree node and link it to the list.
                    struct data_tree *new_root = malloc(sizeof(struct data_tree));
                    new_root->node = NULL;
                    new_root->prev = config_root_tree;
                    config_root_tree = new_root;
                    curr_root = config_root_tree;
                }
            }

            struct lyd_node *new_parent = NULL;


            // check if the node exist in the tree, if not create new node in the tree.
            ret = lyd_find_path(parent_data, xpath, 0, &new_parent);
            if (new_parent == NULL) {
                ret = lyd_new_path(parent_data, sysrepo_ctx, xpath, NULL, LYD_NEW_PATH_UPDATE, &new_parent);
            }

            update_parent:
            // if the edit operation is 'add', then update the parent_node, else (which is 'delete' operation) then just set the out node.
            if (edit_type == EDIT_DATA_ADD)
                parent_data = new_parent;
            else
                *out_node = new_parent;

            curr_root->node = curr_root->node ? curr_root->node : parent_data;
        }
            break;

        case LYS_LEAF:
        case LYS_LEAFLIST:
            if (y_node->nodetype == LYS_LEAFLIST)
                snprintf(xpath, 256, "%s:%s[.='%s']", y_node->module->name, y_node->name, value);
            else
                snprintf(xpath, 256, "%s:%s", y_node->module->name, y_node->name);

            // check if node already exist in data_tree, if not creat a new node.
            ret = lyd_find_path(parent_data, xpath, 0, out_node);

            if (out_node == NULL) {
                ret = lyd_new_path(parent_data, sysrepo_ctx, xpath, value, LYD_NEW_PATH_UPDATE,
                                   out_node);
            }
            break;

    }
    if (ret != LY_SUCCESS)
        print_ly_err(ly_err_first(sysrepo_ctx), "data_factory.c");


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
