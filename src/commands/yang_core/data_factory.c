//
// Created by ali on 11/17/23.
//
#include "data_factory.h"
#include "src/onm_sysrepo.h"
#include "y_utils.h"
#include "src/onm_logger.h"


struct lyd_node *parent_data;
struct data_tree *curr_root;
extern struct data_tree *config_root_tree;

struct data_tree *get_config_root_tree() {
    return config_root_tree;
}

// edit type
enum {
    EDIT_DATA_ADD,
    EDIT_DATA_DEL,
};


// get list data node for y_node from local data_tree.
struct lyd_node *get_local_list_nodes(struct lysc_node *y_node) {
    char xpath[256] = {0};
    struct lyd_node *match = NULL;
    if (parent_data->schema == y_node->parent)
        match = parent_data;
    else {
        sprintf(xpath, "%s", get_relative_path(y_node->parent));
        lyd_find_path(parent_data, xpath, 0, &match);
        // for leafref if node does not exist in parent_data, check all roots.
        if (match == NULL) {
            struct data_tree *curr_node = config_root_tree;
            lysc_path(y_node->parent, LYSC_PATH_DATA, xpath, 256);
            while (curr_node != NULL && curr_node->node != NULL) {
                lyd_find_path(curr_node->node, xpath, 0, &match);
                if (match != NULL)
                    break;
                curr_node = curr_node->prev;  // Move to the next node
            }
        }
    }
    if (match == NULL)
        return NULL;
    struct lyd_node *list_node = lyd_child(match);
    struct lyd_node *next = NULL;
    LY_LIST_FOR(list_node, next) {
        if (next->schema->nodetype == LYS_LIST) {
            if (!strcmp(y_node->name, next->schema->name))
                return next;
        }
    }
    return NULL;
}

// get list data node for y_node from local data_tree, if no data get from sysrepo.
struct lyd_node *get_local_or_sr_list_nodes(struct lysc_node *y_node) {
    struct lyd_node *list_entries = get_local_list_nodes(y_node);
    if (list_entries == NULL) {
        list_entries = sysrepo_get_data_subtree(y_node->parent);
        struct lyd_node *list_node = lyd_child(list_entries);
        struct lyd_node *next = NULL;
        LY_LIST_FOR(list_node, next) {
            if (next->schema->nodetype == LYS_LIST) {
                if (!strcmp(y_node->name, next->schema->name))
                    return next;
            }
        }
    }
    return list_entries;
}

struct lyd_node *get_local_node_data(char *xpath) {
    struct lyd_node *match= NULL;
    struct data_tree *curr_node = config_root_tree;
    while (curr_node != NULL && curr_node->node != NULL) {
        lyd_find_path(curr_node->node, xpath, 0, &match);
        if (match != NULL)
            break;
        curr_node = curr_node->prev;  // Move to the next node
    }
    return match;
}

char *create_list_path_predicate(struct lysc_node *y_node, char *argv[], int argc) {
    const struct lysc_node *child_list = lysc_node_child(y_node);
    const struct lysc_node *child;
    int arg_pos = -1;
    size_t total_len = 0;
    // Calculate the total length needed for the string
//    if (with_module_name)
//        total_len = strlen(y_node->module->name) + strlen(y_node->name) + 2; // +2 for ":", "]", and null terminator

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
        LOG_ERROR("Memory allocation failed");
        return NULL;
    }

//    // Construct the string
//    if (with_module_name)
//        sprintf(predicate_str, "%s:%s", y_node->module->name, y_node->name);
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
                             int index, struct cli_def *cli, int is_update_parent) {
    int ret;
    char xpath[265];
    char *predicate_str;
    memset(xpath, 0, 265);
    struct ly_ctx *sysrepo_ctx = (struct ly_ctx *) sysrepo_get_ctx();
    if (!sysrepo_ctx) {
        LOG_ERROR(" add_data_node(): Failure: failed to get sysrepo_ctx");
        sysrepo_release_ctx();
        return EXIT_FAILURE;
    }

    sprintf(xpath, "%s", get_relative_path(y_node));
    struct lyd_node *curr_parent, *new_parent;
    // set current parent and xpath based on the list location in the tree.
    if (parent_data == NULL) {
        curr_parent = curr_root->node;
        lysc_path(y_node, LYSC_PATH_DATA, xpath, 256);
    } else
        curr_parent = parent_data;

    predicate_str = create_list_path_predicate(y_node, argv, argc);
    strcat(xpath, predicate_str);

    ret = lyd_find_path(curr_parent, xpath, 0, &new_parent);
    if (new_parent == NULL || ret == LY_EINCOMPLETE)
        ret = lyd_new_path2(curr_parent, sysrepo_ctx, xpath, NULL, 0,
                            0, LYD_NEW_PATH_UPDATE, NULL, &new_parent);

    if (index) {
        // index start from 10 and the step is 10, 10,20,30...
        int curr_indx = 10;
        struct lyd_node *next = NULL;
        struct lyd_node *orderd_nodes = lyd_first_sibling(new_parent);

        LY_LIST_FOR(orderd_nodes, next) {
            if (index < curr_indx) {
                ret = lyd_insert_before(next, new_parent);
                if (ret != LY_SUCCESS) {
                    lyd_free_tree(new_parent);
                    goto done;
                }
                break;

            }
            curr_indx += 10;
        }
    }
    if (edit_type == EDIT_DATA_ADD) {
        if (is_update_parent)
            parent_data = new_parent;
    } else
        lyd_free_tree(new_parent);


    done:
    free(predicate_str);
    if (ret != LY_SUCCESS) {
        print_ly_err(ly_err_first(sysrepo_ctx), "data_factory.c", cli);
    }
    sysrepo_release_ctx();
    return ret;
}


int add_data_node_list(struct lysc_node *y_node, char *argv[], int argc, int index, struct cli_def *cli,
                       int has_none_key_nodes) {
    return edit_node_data_tree_list(y_node, argv, argc, EDIT_DATA_ADD, index, cli, has_none_key_nodes);
}

int delete_data_node_list(struct lysc_node *y_node, char *argv[], int argc, struct cli_def *cli) {

    return edit_node_data_tree_list(y_node, argv, argc, EDIT_DATA_DEL, 0, cli, 0);// no index use key for delete

}

struct lyd_node *get_sysrepo_root_node(char *xpath) {
    sr_data_t *sysrepo_subtree;
    int ret = sr_get_subtree(sysrepo_get_session(), xpath, 0, &sysrepo_subtree);
    if (ret == SR_ERR_OK)
        return sysrepo_subtree->tree;
    if (ret == SR_ERR_NOT_FOUND)
        return NULL;
    LOG_ERROR("data_factory.c: error returning sysrepo data, code=%d", ret);
    return NULL;
}

static int edit_node_data_tree(struct lysc_node *y_node, char *value, int edit_type, struct cli_def *cli) {
    int ret;
    char xpath[256];
    memset(xpath, '\0', 256);
    struct ly_ctx *sysrepo_ctx = (struct ly_ctx *) sysrepo_get_ctx();


    if (!sysrepo_ctx) {
        LOG_ERROR(" add_data_node(): Failure: failed to get sysrepo_ctx");
        sysrepo_release_ctx();
        return EXIT_FAILURE;
    }
    switch (y_node->nodetype) {
        case LYS_CHOICE:
            sysrepo_release_ctx();
            return LY_SUCCESS;
        case LYS_CONTAINER: {
            if (y_node->parent != NULL)
                snprintf(xpath, 256, "%s", get_relative_path(y_node));
            else
                lysc_path(y_node, LYSC_PATH_DATA, xpath, 256);
            // set the config_data_tree
            // check if this is first node in the schema, to set the root node.
            if (y_node->parent == NULL) {
                if (config_root_tree == NULL) {
                    config_root_tree = malloc(sizeof(struct data_tree));
                    config_root_tree->node = get_sysrepo_root_node(xpath);
                    config_root_tree->prev = NULL;
                    curr_root = config_root_tree;
                    if (config_root_tree->node != NULL) {
                        parent_data = config_root_tree->node;
                        sysrepo_release_ctx();
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
                            sysrepo_release_ctx();
                            return LY_SUCCESS;
                        }
                        curr_root = curr_root->prev;
                    }
                    // create new root_tree node and link it to the list.
                    struct data_tree *new_root = malloc(sizeof(struct data_tree));
                    new_root->node = get_sysrepo_root_node(xpath);
                    new_root->prev = config_root_tree;
                    config_root_tree = new_root;
                    curr_root = config_root_tree;
                    parent_data = curr_root->node;
                }
            }

            struct lyd_node *new_parent = NULL;


            // check if the node exist in the tree, if not create new node in the tree.
            ret = lyd_find_path(parent_data, xpath, 0, &new_parent);
            if (new_parent == NULL || ret == LY_EINCOMPLETE) {
                ret = lyd_new_path(parent_data, sysrepo_ctx, xpath, NULL, LYD_NEW_PATH_UPDATE, &new_parent);
            }

            // if the edit operation is 'add', then update the parent_node, else (which is 'delete' operation) then just set the out node.
            if (edit_type == EDIT_DATA_ADD)
                parent_data = new_parent;
            else {
                lyd_free_tree(new_parent);
                break;
            }

            curr_root->node = curr_root->node ? curr_root->node : parent_data;
        }
            break;

        case LYS_LEAF:
        case LYS_LEAFLIST:
            if (y_node->nodetype == LYS_LEAFLIST)
                snprintf(xpath, 256, "%s:%s[.='%s']", y_node->module->name, get_relative_path(y_node), value);
            else
                snprintf(xpath, 256, "%s:%s", y_node->module->name, get_relative_path(y_node));


            struct lyd_node *new_leaf;

            // check if node already exist in data_tree, if not creat a new node.
            ret = lyd_find_path(parent_data, xpath, 0, &new_leaf);
            if (new_leaf == NULL || ret == LY_EINCOMPLETE) {
                ret = lyd_new_path(parent_data, sysrepo_ctx, xpath, value, LYD_NEW_PATH_UPDATE,
                                   &new_leaf);
            }


            if (edit_type == EDIT_DATA_ADD)
                lyd_change_term(new_leaf, value);
            else
                lyd_free_tree(new_leaf);
            break;

    }
    if (ret != LY_SUCCESS)
        print_ly_err(ly_err_first(sysrepo_ctx), "data_factory.c", cli);
    sysrepo_release_ctx();

    return ret;
}

int add_data_node(struct lysc_node *y_node, char *value, struct cli_def *cli) {
    // for add node we just create the node in the data tree, we don't need the lyd_node.
    return edit_node_data_tree(y_node, value, EDIT_DATA_ADD, cli);
}

void get_xpath(struct lysc_node *y_node, char xpath[]) {
    // get the parent path
    lyd_path(parent_data, LYD_PATH_STD, xpath, 1024);
    strcat(xpath, "/");
    strcat(xpath, y_node->module->name);
    strcat(xpath, ":");
    strcat(xpath, y_node->name);

}

int delete_data_node(struct lysc_node *y_node, char *value, struct cli_def *cli) {
    return edit_node_data_tree(y_node, value, EDIT_DATA_DEL, cli);
}
