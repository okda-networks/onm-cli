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
#include <string.h>
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

static void parse_schema_mount_args(int argc, char **argv) {
    const char *mod = NULL;
    const char *lab = NULL;

    const char *k_mod_eq = "schema-mount-module=";
    const char *k_lab_eq = "schema-mount-label=";
    const char *k_lable_eq = "schema-mount-lable="; // common misspelling supported

    for (int i = 1; i < argc; ++i) {
        char *arg = argv[i];
        if (strncmp(arg, "--", 2) == 0) arg += 2; // allow leading --

        if (strncmp(arg, k_mod_eq, strlen(k_mod_eq)) == 0) {
            mod = arg + strlen(k_mod_eq);
            continue;
        }
        if (strncmp(arg, k_lab_eq, strlen(k_lab_eq)) == 0) {
            lab = arg + strlen(k_lab_eq);
            continue;
        }
        if (strncmp(arg, k_lable_eq, strlen(k_lable_eq)) == 0) {
            lab = arg + strlen(k_lable_eq);
            continue;
        }

        // Optional support: space-separated form: --key value
        if (strcmp(arg, "schema-mount-module") == 0 && i + 1 < argc) {
            mod = argv[++i];
            continue;
        }
        if ((strcmp(arg, "schema-mount-label") == 0 || strcmp(arg, "schema-mount-lable") == 0) && i + 1 < argc) {
            lab = argv[++i];
            continue;
        }
    }

    if (mod || lab) {
        if (!mod || !lab) {
            fprintf(stderr, "[WARN] schema-mount args incomplete: module=%s label=%s\n",
                    mod ? mod : "(none)", lab ? lab : "(none)");
            fflush(stderr);
            LOG_WARNING("schema-mount args incomplete: module=%s label=%s", mod ? mod : "(none)", lab ? lab : "(none)");
        } else {
            fprintf(stderr, "[INF] schema-mount args: module=%s label=%s\n", mod, lab);
            fflush(stderr);
            LOG_INFO("schema-mount args: module=%s label=%s", mod, lab);
        }
        sysrepo_set_schema_mount_args(mod, lab);
    }
}

int main(int argc, char **argv) {
    int ret;

    ret = onm_logger_init();
    if (ret != EXIT_SUCCESS) {
        LOG_ERROR("failed to initialize logger: existing...");
        return -1;
    }

    // Parse optional schema-mount CLI args early
    parse_schema_mount_args(argc, argv);

    printf("[INF] Loading Sysrepo3 modules... This may take time.\n");

    ret = onm_sysrepo_init();
    if (ret != EXIT_SUCCESS) {
        LOG_ERROR("failed to initialize yang context: existing...");
        return -1;
    }

    ret = onm_cli_init();
    if (ret != EXIT_SUCCESS) {
        LOG_ERROR("failed to initialize cli: existing...");
        return -1;
    }

    int fd = dup(STDIN_FILENO);
    set_input_mode();
    handle_session(fd);

    // cleanup
    onm_cli_done();
    onm_sysrepo_done();

    LOG_INFO("onmcli exiting...");
    return 0;
}
