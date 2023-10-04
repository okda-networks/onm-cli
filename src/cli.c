//
// Created by ali on 10/4/23.
//
#include "cli.h"
#include "commands.h"
#include "VERSION.h"




struct cli_def *cli;


int idle_timeout(struct cli_def *cli_) {
    cli_print(cli, "session timeout existing..\n");
    return CLI_QUIT;
}

int handle_session(int fd){
    return cli_loop(cli,fd);

}

int onm_cli_init() {
    cli = cli_init();
    char banner[100];
    sprintf(banner, "\n\nonm-cli version: %d.%d.%d\n\nby Okda networks (c) 2023", MAJOR, MINOR, PATCH);

    cli_set_banner(cli, banner);
    cli_set_hostname(cli, "dentos_r01");


    cli_set_idle_timeout_callback(cli, CLI_TIMEOUT, idle_timeout);

    onm_commands_init(cli);


}
