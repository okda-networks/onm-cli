//
// Created by ali on 11/19/23.
//

#ifndef ONMCLI_ONM_SYSREPO_H
#define ONMCLI_ONM_SYSREPO_H

#include <sysrepo.h>
#include "lib/libcli/libcli.h"

struct data_tree {
    struct lyd_node *node;
    struct data_tree *prev;
};


int sysrepo_release_ctx();

int sysrepo_discard_changes();

int sysrepo_has_uncommited_changes(struct lyd_node *data_node);

int sysrepo_commit(struct lyd_node *data_tree);

int onm_sysrepo_init();

int onm_sysrepo_done();

char *sysrepo_get_error_msg();

int sysrepo_insmod(char *mod);

int sysrepo_rmmod(char *mod, int force);

struct lyd_node *sysrepo_get_data_subtree(struct lysc_node *y_node);

const struct ly_ctx *sysrepo_get_ctx();

sr_session_ctx_t *sysrepo_get_session();

sr_session_ctx_t *sysrepo_get_session_startup();

void sysrepo_set_module_path(char *path);

void free_data_tree(struct data_tree *dtree);

void free_data_tree_all();

#endif //ONMCLI_ONM_SYSREPO_H
