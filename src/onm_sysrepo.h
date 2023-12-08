//
// Created by ali on 11/19/23.
//

#ifndef ONMCLI_ONM_SYSREPO_H
#define ONMCLI_ONM_SYSREPO_H

#include <sysrepo.h>

const struct ly_ctx *sysrepo_get_ctx();

sr_session_ctx_t *sysrepo_get_session();

int sysrepo_release_ctx();

int sysrepo_discard_changes();

int sysrepo_commit(struct lyd_node *data_tree);

int sysrepo_init();

#endif //ONMCLI_ONM_SYSREPO_H
