//
// Created by ali on 10/5/23.
//

#ifndef ONM_UTILS_H
#define ONM_UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "lib/libcli/libcli.h"

unsigned int str2int_hash(char *str, ...) ;
void str2fun_name(char *str);
char * create_func_name(char*name1,char*name2,char*name3);
void to_lower(char *str);
void create_argv_from_optpair(struct cli_optarg_pair *head, char ***argv, int *argc);
#endif //ONM_UTILS_H
