//
// Created by ali on 10/21/23.
//
#include "y_utils.h"
#include "src/onm_logger.h"
#define CONFIG_MODE 1


void print_ly_err(const struct ly_err_item *err, char *component, struct cli_def *cli) {

    while (err) {
        if (err->level == LY_LLERR)
            cli_print(cli,"ERROR: YANG: %s",err->msg);
        LOG_ERROR(":%s:libyang error: %s\n",component, err->msg);
        err = err->next;
    }
}

int y_get_curr_mode(struct lysc_node *y_node) {
    unsigned int mode;
    if (y_node->parent == NULL)
        mode = CONFIG_MODE;
    else {
        char xpath[256];
        lysc_path(y_node->parent, LYSC_PATH_LOG, xpath, 256);
        mode = str2int_hash(xpath, NULL);
    }
    return mode;
}

int y_get_next_mode(struct lysc_node *y_node) {
    char xpath[256];
    lysc_path(y_node, LYSC_PATH_LOG, xpath, 256);
    unsigned int mode = str2int_hash(xpath, NULL);
    return mode;
}

size_t calculate_identities_length(struct lysc_ident *identity) {
    size_t length = 0;

    if (!identity) {
        return 0;
    }
    LY_ARRAY_COUNT_TYPE i;


    LY_ARRAY_FOR(identity->derived, i) {
        length += strlen(" [+] ") + strlen(identity->derived[i]->module->name) + strlen(identity->derived[i]->name) + 2;
        // Recursively calculate derived identities
        length += calculate_identities_length(identity->derived[i]);
    }

    return length;
}

void add_identities_recursive(struct lysc_ident *identity, char *help) {
    if (!identity) {
        return;
    }
    LY_ARRAY_COUNT_TYPE i;


    LY_ARRAY_FOR(identity->derived, i) {
        char * id_str = malloc(strlen(identity->derived[i]->name) + strlen(identity->derived[i]->module->name)+2);
        sprintf(id_str,"%s:%s",identity->derived[i]->module->name,identity->derived[i]->name);
        strcat(help, " [+] ");
        strcat(help, id_str);
        strcat(help, "\n");

        // Recursively call to print derived identities
        add_identities_recursive(identity->derived[i], help);
    }
}

const char *creat_help_for_identity_type(struct lysc_node *y_node) {
    const char *y_dsc = y_node->dsc;

    struct lysc_type_identityref *y_id_ref = (struct lysc_type_identityref *) ((struct lysc_node_leaf *) y_node)->type;
//    if (y_id_ref==NULL)
//        return y_node->dsc;
    struct lysc_ident **identities = y_id_ref->bases;
    LY_ARRAY_COUNT_TYPE i;

    // Allocate memory for the help string
    size_t help_len = strlen(y_dsc) + strlen("Available options:\n");

    LY_ARRAY_FOR(identities, i) {
        help_len += strlen("[+] ") + strlen(identities[i]->module->name) + strlen(identities[i]->name) +
                    2; // +2 for newline and ":"
        help_len += calculate_identities_length(identities[i]); // Calculate additional length
    }

    char *help = (char *) malloc(help_len);
    if (!help) {
        LOG_ERROR("Memory allocation failed");
        return NULL;
    }

    // Create the string
    strcpy(help, y_dsc);
    strcat(help, "\nAvailable options:\n");

    LY_ARRAY_FOR(identities, i) {
        add_identities_recursive(identities[i], help);
    }
    return help;
}