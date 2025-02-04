/* SPDX-License-Identifier: AGPL-3.0-or-later */
/*
 * Authors:     Ali Aqrabawi, <aaqrbaw@okdanetworks.com>
 *
 *              This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU Affero General Public
 *              License Version 3.0 as published by the Free Software Foundation;
 *              either version 3.0 of the License, or (at your option) any later
 *              version.
 *
 * Copyright (C) 2024 Okda Networks, <aaqrbaw@okdanetworks.com>
 */

#include "data_factory.h"
#include "src/onm_sysrepo.h"
#include "y_utils.h"
#include "src/onm_logger.h"


struct lyd_node *parent_data = NULL;

void free_parent_data() {
    lyd_free_tree(parent_data);
    parent_data = NULL;
}

struct lyd_node *get_current_parent_dnode() {
    return parent_data;
}

// edit type
enum {
    EDIT_DATA_ADD,
    EDIT_DATA_DEL,
};


struct lyd_node *get_sysrepo_candidate_node(char *xpath) {
    sr_data_t *sysrepo_subtree;
    int ret = sr_get_subtree(sysrepo_get_session(), xpath, 0, &sysrepo_subtree);
    if (ret == SR_ERR_OK && sysrepo_subtree != NULL)
        return sysrepo_subtree->tree;

    if (ret == SR_ERR_NOT_FOUND)
        return NULL;
    LOG_ERROR("data_factory.c: error returning sysrepo data, code=%d", ret);
    return NULL;
}


struct lyd_node *get_sysrepo_running_node(char *xpath) {
    sr_data_t *sysrepo_subtree;
    int ret = sr_get_subtree(sysrepo_get_running_session(), xpath, 0, &sysrepo_subtree);
    if (ret == SR_ERR_OK && sysrepo_subtree != NULL)
        return sysrepo_subtree->tree;
    if (ret == SR_ERR_NOT_FOUND)
        return NULL;
    LOG_ERROR("data_factory.c: error returning sysrepo data, code=%d", ret);
    return NULL;
}

struct lyd_node *get_sysrepo_operational_node(char *xpath) {
    sr_data_t *sysrepo_subtree;
    int ret = sr_get_subtree(sysrepo_get_session_operational(), xpath, 0, &sysrepo_subtree);
    if (ret == SR_ERR_OK && sysrepo_subtree != NULL)
        return sysrepo_subtree->tree;
    if (ret == SR_ERR_NOT_FOUND)
        return NULL;
    LOG_ERROR("data_factory.c: error returning sysrepo data, code=%d", ret);
    return NULL;
}

struct lyd_node *get_sysrepo_startup_node(char *xpath) {
    sr_data_t *sysrepo_subtree;
    int ret = sr_get_subtree(sysrepo_get_session_startup(), xpath, 0, &sysrepo_subtree);
    if (ret == SR_ERR_OK && sysrepo_subtree != NULL)
        return sysrepo_subtree->tree;
    if (ret == SR_ERR_NOT_FOUND)
        return NULL;
    LOG_ERROR("data_factory.c: error returning sysrepo data, code=%d", ret);
    return NULL;
}


int edit_node_data_tree_list(struct lysc_node *y_node, int edit_type,
                             int index, struct cli_def *cli, int is_update_parent) {
    int ret = EXIT_SUCCESS;
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
    struct lyd_node *curr_parent = NULL, *new_parent = NULL;
    // set current parent and xpath based on the list location in the tree.
    if (parent_data == NULL) {
        lysc_path(y_node, LYSC_PATH_DATA, xpath, 256);
    } else
        curr_parent = parent_data;

    predicate_str = create_list_predicate_from_optargs(cli, y_node);
    strcat(xpath, predicate_str);

    char all_xpath[1024] = {0};
    lyd_path(parent_data, LYD_PATH_STD, all_xpath, 1024);
    strlcat(all_xpath, "/", sizeof(all_xpath));
    strlcat(all_xpath, xpath, sizeof(all_xpath));

    int item_found = 1;
    new_parent = get_sysrepo_candidate_node(all_xpath);
    if (new_parent == NULL) {
        item_found = 0;
        ret = lyd_new_path2(curr_parent, sysrepo_ctx, all_xpath, NULL, 0,
                            0, LYD_NEW_PATH_UPDATE, NULL, &new_parent);
    }

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
        if (!item_found) {
            ret = sr_set_item(sysrepo_get_session(), all_xpath, NULL, 0);
            if (ret != SR_ERR_OK)
                goto done;
        }
        if (is_update_parent)
            parent_data = new_parent;
    } else {
        if (item_found) {
            ret = sr_delete_item(sysrepo_get_session(), all_xpath, 0);
            if (ret != SR_ERR_OK)
                goto done;
        } else
            cli_print(cli, " item not found in datastore!");
        lyd_free_tree(new_parent);
    }


done:
    if (ret != LY_SUCCESS) {
        print_ly_err(ly_err_first(sysrepo_ctx), "data_factory.c", cli);
    }
    free(predicate_str);
    sysrepo_release_ctx();
    return ret;
}


int add_data_node_list(struct lysc_node *y_node, int index, struct cli_def *cli,
                       int has_none_key_nodes) {
    return edit_node_data_tree_list(y_node, EDIT_DATA_ADD, index, cli, has_none_key_nodes);
}

int delete_data_node_list(struct lysc_node *y_node, struct cli_def *cli) {
    return edit_node_data_tree_list(y_node, EDIT_DATA_DEL, 0, cli, 0); // no index use key for delete
}


static int edit_node_data_tree(struct lysc_node *y_node, char *value, int edit_type, struct cli_def *cli) {
    int ret = LY_SUCCESS;
    char xpath[256];
    memset(xpath, '\0', 256);
    struct ly_ctx *sysrepo_ctx = (struct ly_ctx *) sysrepo_get_ctx();
    char all_xpath[1024] = {0};

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

            struct lyd_node *new_parent = NULL;

            int item_found = 1;
            // check if the node exist in the tree, if not create new node in the tree.
            ret = lyd_find_path(parent_data, xpath, 0, &new_parent);
            if (ret == LY_ENOTFOUND)
                item_found = 0;
            if (new_parent == NULL || ret == LY_EINCOMPLETE) {
                ret = lyd_new_path(parent_data, sysrepo_ctx, xpath, NULL, LYD_NEW_PATH_UPDATE, &new_parent);
            }
            if (ret != LY_SUCCESS)
                break;
            // if the edit operation is 'add', then update the parent_node, else (which is 'delete' operation) then just set the out node.
            if (edit_type == EDIT_DATA_ADD)
                parent_data = new_parent;
            else {
                lyd_free_tree(new_parent);
                if (item_found) {
                    lyd_path(parent_data, LYD_PATH_STD, all_xpath, 1024);
                    strlcat(all_xpath, "/", sizeof(all_xpath));
                    strlcat(all_xpath, xpath, sizeof(all_xpath));
                    ret = sr_delete_item(sysrepo_get_session(), all_xpath, 0);
                    if (ret != SR_ERR_OK)
                        break;
                } else
                    cli_print(cli, " item not found in datastore!");
                break;
            }
        }
        break;

        case LYS_LEAF:
        case LYS_LEAFLIST:
            if (y_node->nodetype == LYS_LEAFLIST)
                snprintf(xpath, 256, "%s[.='%s']", get_relative_path(y_node), value);
            else
                snprintf(xpath, 256, "%s", get_relative_path(y_node));

            struct lyd_node *new_leaf = NULL;
            int item_found = 1;
            lyd_path(parent_data, LYD_PATH_STD, all_xpath, 1024);
            strlcat(all_xpath, "/", sizeof(all_xpath));
            strlcat(all_xpath, xpath, sizeof(all_xpath));

            new_leaf = get_sysrepo_candidate_node(all_xpath);
            if (new_leaf == NULL) {
                item_found = 0;
                ret = lyd_new_path2(parent_data, sysrepo_ctx, xpath, value, strlen(value), LYD_ANYDATA_STRING,
                                    LYD_NEW_PATH_OUTPUT, NULL, &new_leaf);
            }


            if (edit_type == EDIT_DATA_ADD) {
                if (!item_found || strcmp(lyd_get_value(new_leaf), value) != 0) {
                    // SR_EDIT_ISOLATE is needed to change the edit value of leaf,
                    // for example we set the value of mtu to 1300, then we change it to 1200 without commit,
                    // for this case SR_EDIT_ISOLATE is required.
                    ret = sr_set_item_str(sysrepo_get_session(), all_xpath, value, NULL, SR_EDIT_ISOLATE);
                    if (ret != SR_ERR_OK)
                        break;
                }
            } else {
                lyd_free_tree(new_leaf);
                if (item_found) {
                    ret = sr_delete_item(sysrepo_get_session(), all_xpath, 0);
                    if (ret != SR_ERR_OK)
                        break;
                } else
                    cli_print(cli, " item not found in datastore!");
            }
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


int delete_data_node(struct lysc_node *y_node, char *value, struct cli_def *cli) {
    return edit_node_data_tree(y_node, value, EDIT_DATA_DEL, cli);
}
