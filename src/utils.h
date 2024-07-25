/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef ONM_UTILS_H
#define ONM_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "lib/libcli/libcli.h"

// defnie colores.
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

unsigned int str2int_hash(char *str, ...);

void str2fun_name(char *str);

char *create_func_name(char *name1, char *name2, char *name3);

void to_lower(char *str);


#endif //ONM_UTILS_H
