//
// Created by ali on 10/4/23.
//
#include <stdio.h>
#include <unistd.h>
#include "onm_cli.h"
#include "commands/default_cmd.h"
#include "commands/yang_loader_cmd.h"
#include "VERSION.h"


struct cli_def *cli;


int idle_timeout(struct cli_def *cli_) {
    cli_print(cli, "session timeout existing..\n");
    return CLI_QUIT;
}

int handle_session(int fd) {
    return cli_loop(cli, fd);

}

int onm_cli_init() {
    cli = cli_init();
    char banner[100]= "router";
    sprintf(banner, "\n\nonmcli version: %d.%d.%d\nby Okda networks (c) 2023", MAJOR, MINOR, PATCH);

    cli_set_banner(cli, banner);
    char hostname[256];

    if (gethostname(hostname, sizeof(hostname)) == 0) {
        printf("Hostname: %s\n", hostname);
    } else {
        perror("gethostname:");

    }
    cli_set_hostname(cli, hostname);

    cli_set_idle_timeout_callback(cli, CLI_TIMEOUT, idle_timeout);

    default_commands_init(cli);
    yang_cmd_loader_init(cli);
    return 0;


}
