/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef ONMCLI_ONM_SYSREPO_H
#define ONMCLI_ONM_SYSREPO_H

#include <sysrepo.h>
#include "lib/libcli/libcli.h"

int sysrepo_release_ctx();

int sysrepo_discard_changes();

int sysrepo_commit();

int onm_sysrepo_init();

int onm_sysrepo_done();

char *sysrepo_get_error_msg();

int sysrepo_insmod(char *mod);

int sysrepo_rmmod(char *mod, int force);

struct lyd_node *sysrepo_get_data_subtree(const char *path);

const struct ly_ctx *sysrepo_get_ctx();

sr_session_ctx_t *sysrepo_get_session();

sr_session_ctx_t *sysrepo_get_running_session();

sr_session_ctx_t *sysrepo_get_session_startup();

sr_session_ctx_t *sysrepo_get_session_operational();

void sysrepo_set_module_path(char *path);

#endif //ONMCLI_ONM_SYSREPO_H
