//
// Created by ali on 10/21/23.
//
#include "y_utils.h"
#include "src/onm_logger.h"
#include "uthash.h"

#define CONFIG_MODE 1

enum step_action {
    STEP_FORWARD,
    STEP_BACKWARD
};

void config_print(struct cli_def *cli, struct lyd_node *d_node) {
    char *result;
    lyd_print_mem(&result, d_node, LYD_XML, 0);
    cli_print(cli, result, NULL);
}

const char *get_relative_path(struct lysc_node *y_node) {
    struct lysc_node *root_parent = y_node->parent;
    while (root_parent != NULL && root_parent->nodetype != LYS_LIST) {
        if (root_parent->parent == NULL)
            break;
        root_parent = root_parent->parent;
    }

    char xpath_parent[256], xpath_child[256];
    memset(xpath_parent, 0, sizeof(xpath_parent));
    memset(xpath_child, 0, sizeof(xpath_child));

    lysc_path(y_node, LYSC_PATH_DATA, xpath_child, 256);
    lysc_path(root_parent, LYSC_PATH_DATA, xpath_parent, 256);
    // Find the position where x and y differ
    size_t i;
    for (i = 0; xpath_parent[i] != '\0' && xpath_child[i] != '\0' && xpath_parent[i] == xpath_child[i]; ++i);

    // Extract the remaining part of y
    int shift = 0;
    if (xpath_child[i] == '/')
        shift = 1;
    const char *result = &xpath_child[i + shift];

    // Print the result
    return strdup(result);
}


struct cli_command *search_cmds(struct cli_command *commands, struct lysc_node **y_node) {
    struct cli_command *c;
    const char *root_module = lysc_owner_module(*y_node)->name;

    for (c = commands; c; c = c->next) {
        if (c->command_hash == NULL) continue;
        if (strcmp(c->command_hash, root_module) != 0) continue;
        if (c->children) {
            struct cli_command *found_c = search_cmds(c->children, y_node);

            if (found_c != NULL) {
                return found_c;
            }
        }
        if (!strcmp((*y_node)->name, "interfaces") && !strcmp(c->command, "diff"))
            printf("here");
        if ((struct lysc_node *) c->cmd_model != *y_node) continue;
        if (strcmp(c->command, (*y_node)->name) != 0) continue;
        return c;
    }

    return NULL;
}

//struct cli_command *get_cli_yang_command(struct cli_def *cli, struct lysc_node **y_node) {
//    struct cli_command * no_cmd = ((struct cli_ctx_data *)cli_get_context(cli))->no_cmd;
//    return search_cmds(cli->commands, y_node,no_cmd);
//}

typedef enum {
    CONFIG_PARENT,
    NO_CONFIG_PARENT,
    SHOW_CONFIG_CANDIDATE_PARENT,
    SHOW_CONFIG_RUNNING_PARENT,
    SHOW_CONFIG_STARTUP_PARENT,
} FIND_PARENT_T;

struct cli_command *find_parent_command(struct cli_def *cli, struct lysc_node *y_node, FIND_PARENT_T parent_type) {
    struct cli_command *root_cmds;

    switch (parent_type) {
        case CONFIG_PARENT:
            root_cmds = cli->commands;
            break;
        case NO_CONFIG_PARENT:
            root_cmds = ((struct cli_ctx_data *) cli_get_context(cli))->no_cmd->children;
            break;
        case SHOW_CONFIG_CANDIDATE_PARENT:
            root_cmds = ((struct cli_ctx_data *) cli_get_context(cli))->show_conf_cand_cmd->children;
            if (y_node->parent != NULL)
                return search_cmds(root_cmds, &y_node->parent);
        case SHOW_CONFIG_RUNNING_PARENT:
            root_cmds = ((struct cli_ctx_data *) cli_get_context(cli))->show_conf_running_cmd->children;
            if (y_node->parent != NULL)
                return search_cmds(root_cmds, &y_node->parent);
        case SHOW_CONFIG_STARTUP_PARENT:
            root_cmds = ((struct cli_ctx_data *) cli_get_context(cli))->show_conf_startup_cmd->children;
            if (y_node->parent != NULL)
                return search_cmds(root_cmds, &y_node->parent);
            break;
    }


    if (y_node->parent != NULL && y_node->parent->parent != NULL
        && (y_node->parent->nodetype == LYS_CONTAINER || y_node->parent->nodetype == LYS_CHOICE ||
            y_node->parent->nodetype == LYS_CASE)) {
        if (!strcmp(y_node->parent->name, y_node->parent->parent->name))
            return search_cmds(root_cmds, &y_node->parent->parent);
        else
            return search_cmds(root_cmds, &y_node->parent);
    }
    return NULL;
}

struct cli_command *find_parent_cmd(struct cli_def *cli, struct lysc_node *y_node) {
    return find_parent_command(cli, y_node, CONFIG_PARENT);
}

struct cli_command *find_parent_no_cmd(struct cli_def *cli, struct lysc_node *y_node) {
    return find_parent_command(cli, y_node, NO_CONFIG_PARENT);
}

struct cli_command *find_parent_show_candidate_cmd(struct cli_def *cli, struct lysc_node *y_node) {
    return find_parent_command(cli, y_node, SHOW_CONFIG_CANDIDATE_PARENT);
}

struct cli_command *find_parent_show_running_cmd(struct cli_def *cli, struct lysc_node *y_node) {
    return find_parent_command(cli, y_node, SHOW_CONFIG_RUNNING_PARENT);
}

struct cli_command *find_parent_show_startup_cmd(struct cli_def *cli, struct lysc_node *y_node) {
    return find_parent_command(cli, y_node, SHOW_CONFIG_STARTUP_PARENT);
}


int is_root_node(const struct lysc_node *y_node) {
    if (y_node->parent == NULL)
        return 1;
    return 0;
}

void print_ly_err(const struct ly_err_item *err, char *component, struct cli_def *cli) {

    while (err) {
        if (err->level == LY_LLERR)
            cli_print(cli, "ERROR: YANG: %s", err->msg);
        LOG_ERROR(":%s:libyang error: %s\n", component, err->msg);
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
        char *id_str = malloc(strlen(identity->derived[i]->name) + strlen(identity->derived[i]->module->name) + 2);
        sprintf(id_str, "%s:%s", identity->derived[i]->module->name, identity->derived[i]->name);
        strcat(help, " [+] ");
        strcat(help, id_str);
        strcat(help, "\n");
        free(id_str);

        // Recursively call to print derived identities
        add_identities_recursive(identity->derived[i], help);
    }
}

const char *creat_help_for_identity_type(struct lysc_node *y_node) {
    const char *y_dsc = y_node->dsc;

    struct lysc_type_identityref *y_id_ref = (struct lysc_type_identityref *) ((struct lysc_node_leaf *) y_node)->type;

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
