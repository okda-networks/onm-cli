/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef ONMCLI_DATA_CMD_COMPL_H
#define ONMCLI_DATA_CMD_COMPL_H

#include "y_utils.h"

int optagr_get_compl_candidate(struct cli_def *cli, const char *name, const char *word, struct cli_comphelp *comphelp,
                               void *cmd_model);

int optagr_get_compl_running(struct cli_def *cli, const char *name, const char *word, struct cli_comphelp *comphelp,
                             void *cmd_model);

int optagr_get_compl_startup(struct cli_def *cli, const char *name, const char *word, struct cli_comphelp *comphelp,
                             void *cmd_model);

int optagr_get_compl_candidate_running(struct cli_def *cli, const char *name, const char *word, struct cli_comphelp *comphelp,
                                       void *cmd_model);

#endif //ONMCLI_DATA_CMD_COMPL_H
