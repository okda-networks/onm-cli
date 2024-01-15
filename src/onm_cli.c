//
// Created by ali on 10/4/23.
//
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

int onm_cli_done(){
    return cli_done(cli);
}

int onm_cli_init() {
    cli = cli_init();
    char banner[1024];
    memset(banner,'\0',1024);
    sprintf(banner, "\n\nonmcli version: %d.%d.%d\nby Okda networks (c) 2023", MAJOR, MINOR, PATCH);

    cli_telnet_protocol(cli, 0);
    cli_set_banner(cli, banner);
    char hostname[64];

    if (gethostname(hostname, sizeof(hostname)) < 0)
        LOG_ERROR("gethostname:");

    cli_set_hostname(cli, hostname);

    cli_set_idle_timeout_callback(cli, CLI_TIMEOUT, idle_timeout);

    struct cli_ctx_data *cli_ctx= malloc(sizeof (struct cli_ctx_data));
    cli_set_context(cli,cli_ctx);
    default_commands_init(cli);
    yang_cmd_loader_init(cli);
    return EXIT_SUCCESS;


}
