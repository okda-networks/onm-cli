//
// Created by ali on 10/5/23.
//

#include "utils.h"
#include <stdarg.h>
#include "onm_logger.h"

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

    return (unsigned int)hash;
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

void create_argv_from_optpair(struct cli_optarg_pair *head, char ***argv, int *argc) {
    int count = count_optargs(head);

    *argv = (char **) malloc((count + 1) * sizeof(char *));  // +1 for the NULL pointer at the end
    if (*argv == NULL) {
        LOG_ERROR("Memory allocation error");
        exit(EXIT_FAILURE);
    }

    *argc = count;

    for (int i = 0; i < count; i++) {
        (*argv)[i] = strdup(head->value);
        head = head->next;
    }
    (*argv)[count] = NULL;  // NULL-terminate the array
}