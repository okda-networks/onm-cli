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
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <termios.h>

#include "config.h"
#include "onm_cli.h"
#include "onm_sysrepo.h"
#include "onm_logger.h"


#define GET_NEW_MODE_STR(current_mod, new_mode) \
    strcat((char*)current_mod, strcat(":", new_mode))

/* Saves the original terminal attributes. */
struct termios saved_termios;

void reset_input_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_termios);
}

void set_input_mode(void) {
    struct termios tattr;

    /* Make sure stdin is a terminal. */
    if (!isatty(STDIN_FILENO)) {
        LOG_ERROR("Not a terminal.");
        exit(EXIT_FAILURE);
    }

    /* Save the terminal attributes so we can restore them later. */
    tcgetattr(STDIN_FILENO, &saved_termios);
    atexit(reset_input_mode);

    /* Set the funny terminal modes. */
    tcgetattr(STDIN_FILENO, &tattr);
    tattr.c_lflag &= ~(ICANON | ECHO);       /* Clear ICANON and ECHO. */
    tattr.c_iflag &= ~(ICRNL);             /* Clear ICRNL. */
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);

}

int main() {
    int ret;

    ret = onm_cli_init();
    if (ret != EXIT_SUCCESS) {
        LOG_ERROR("failed to initialize cli: existing...");
        return -1;
    }

    ret = onm_sysrepo_init();
    if (ret != EXIT_SUCCESS) {
        LOG_ERROR("failed to initialize yang context: existing...");
        return -1;
    }

    ret = onm_logger_init();
    if (ret != EXIT_SUCCESS) {
        LOG_ERROR("failed to initialize logger: existing...");
        return -1;
    }
    int fd = dup(STDIN_FILENO);
    set_input_mode();
    handle_session(fd);

    // cleanup
    onm_cli_done();
    onm_sysrepo_done();

    LOG_INFO("onmcli exiting...");

}
