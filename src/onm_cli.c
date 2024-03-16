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

#include <stdio.h>
#include <unistd.h>
#include "onm_cli.h"
#include "commands/default_cmd.h"
#include "commands/sysrepo_cmd.h"
#include "VERSION.h"
#include "onm_logger.h"


struct cli_def *cli;


int idle_timeout(struct cli_def *cli_) {
    cli_print(cli, "session timeout existing..\n");
    return CLI_QUIT;
}

int handle_session(int fd) {
    return cli_loop(cli, fd);
}

int onm_cli_done() {
    return cli_done(cli);
}

int onm_cli_init() {
    cli = cli_init();
    char banner[1024];
    memset(banner, '\0', 1024);
    sprintf(banner, "\n\nonmcli version: %d.%d.%d\nby Okda networks (c) 2024", MAJOR, MINOR, PATCH);

    cli_telnet_protocol(cli, 0);
    cli_set_banner(cli, banner);
    char hostname[64];

    if (gethostname(hostname, sizeof(hostname)) < 0)
        LOG_ERROR("gethostname:");

    cli_set_hostname(cli, hostname);

    cli_set_idle_timeout_callback(cli, CLI_TIMEOUT, idle_timeout);

    struct cli_ctx_data *cli_ctx = malloc(sizeof(struct cli_ctx_data));
    cli_set_context(cli, cli_ctx);
    default_commands_init(cli);
    sysrepo_commands_init(cli);
    return EXIT_SUCCESS;


}
