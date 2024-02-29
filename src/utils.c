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

#include "utils.h"
#include <stdarg.h>

void to_lower(char *str) {
    while (*str) {
        *str = tolower(*str);
        str++;
    }
}


unsigned int str2int_hash(char *str, ...) {
    unsigned long hash = 5381;
    int c;
    va_list args;
    char *arg;

    // Initialize the argument list
    va_start(args, str);

    // Process the first string argument
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    // Process each optional string argument
    while ((arg = va_arg(args, char *)) != NULL) {
        while ((c = *arg++)) {
            hash = ((hash << 5) + hash) + c;
        }
    }

    // Clean up the argument list
    va_end(args);

    return (unsigned int) hash;
}

void str2fun_name(char *str) {
    while (*str) {
        if (!isalnum(*str)) {
            *str = '_';
        } else {
            *str = tolower(*str);
        }
        str++;
    }
}


int count_optargs(struct cli_optarg_pair *head) {
    int count = 0;
    while (head != NULL) {
        count++;
        head = head->next;
    }
    return count;
}

void free_argv(char **argv, int argc) {
    if (argv == NULL) {
        return;  // Nothing to free
    }

    for (int i = 0; i < argc; i++) {
        free(argv[i]);  // Free each argument
    }

    free(argv);  // Free the array itself
}
