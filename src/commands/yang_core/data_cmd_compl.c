//
// Created by ali on 1/12/24.
//

#include "data_cmd_compl.h"
#include "src/onm_logger.h"
#include "data_factory.h"

int identityref_add_comphelp(struct lysc_ident *identity, const char *word, struct cli_comphelp *comphelp) {
    if (!identity) {
        return 0;
    }
    char **id_entry;
    LY_ARRAY_COUNT_TYPE i;
    LY_ARRAY_FOR(identity->derived, i) {
        char *id_str = malloc(strlen(identity->derived[i]->name) + strlen(identity->derived[i]->module->name) + 2);
        sprintf(id_str, "%s:%s", identity->derived[i]->module->name, identity->derived[i]->name);

        id_entry = &id_str;
        if (!word || !strncmp(*id_entry, word, strlen(word))) {
            cli_add_comphelp_entry(comphelp, *id_entry);
        }
        free(id_str);
        identityref_add_comphelp(identity->derived[i], word, comphelp);
    }
    return EXIT_SUCCESS;
}


void free_options(const char **options) {
    if (options) {
        for (int i = 0; options[i] != NULL; i++) {
            free((void *) options[i]);  // Correctly cast to void* for freeing
        }
        free(options);
    }
}


const char **create_type_options(struct lysc_node *y_node) {
    LY_DATA_TYPE type = ((struct lysc_node_leaf *) y_node)->type->basetype;
    int num_args = 0;
    const char **env_vars = NULL;
    LY_ARRAY_COUNT_TYPE i_sized;
    if (type == LY_TYPE_ENUM) {
        struct lysc_type_enum *y_enum_type = (struct lysc_type_enum *) ((struct lysc_node_leaf *) y_node)->type;
        // Populate the array with enum names
        LY_ARRAY_FOR(y_enum_type->enums, i_sized) {
            num_args++;
            env_vars = realloc(env_vars, sizeof(env_vars) * num_args);
            env_vars[num_args - 1] = strdup(y_enum_type->enums[i_sized].name);
        }
    } else if (type == LY_TYPE_BOOL) {
        env_vars = malloc(3 * sizeof(const char *));
        env_vars[0] = strdup("false");
        env_vars[1] = strdup("true");
        env_vars[2] = NULL;
        return env_vars;
    } else if (type == LY_TYPE_STRING && lysc_is_key(y_node)) {
        struct lyd_node *list_data_node = get_list_nodes(y_node->parent);
        struct lyd_node *next = NULL;
        LY_LIST_FOR(list_data_node, next) {
            struct lyd_node *entry_children = lyd_child(next);
            struct lyd_node *entry_child = NULL;
            LY_LIST_FOR(entry_children, entry_child) {
                if (lysc_is_key(entry_child->schema) && !strcmp(entry_child->schema->name, y_node->name)) {
                    num_args++;
                    env_vars = realloc(env_vars, sizeof(env_vars) * num_args);
                    env_vars[num_args - 1] = strdup(lyd_get_value(entry_child));
                }
            }
        }
    }
    // add null termination for the array
    if (env_vars != NULL) {
        env_vars = realloc(env_vars, sizeof(env_vars) * (num_args + 1));
        env_vars[num_args] = NULL;
    }

    return env_vars;

}


int optagr_get_compl(struct cli_def *cli, const char *name, const char *word, struct cli_comphelp *comphelp,
                     void *cmd_model) {
    if (cmd_model == NULL)
        return 0;
    struct lysc_node *y_node = (struct lysc_node *) cmd_model;
    LY_DATA_TYPE type = ((struct lysc_node_leaf *) y_node)->type->basetype;

    const char **next_option, **options;
    LY_ARRAY_COUNT_TYPE i;
    int rc = CLI_OK;
    // LY_TYPE_IDENT has special case where we need to add recursively.
    if (type == LY_TYPE_IDENT) {
        struct lysc_type_identityref *y_id_type = (struct lysc_type_identityref *) ((struct lysc_node_leaf *) y_node)->type;
        LY_ARRAY_FOR(y_id_type->bases, i) {
            identityref_add_comphelp(y_id_type->bases[i], word, comphelp);
        }
        return CLI_OK;
    }
    options = (const char **) create_type_options(y_node);
    if (options == NULL) {
        LOG_DEBUG("failed to get available options for node %s", y_node->name);
        return CLI_OK;
    }
    for (next_option = options; *next_option && (rc == CLI_OK); next_option++) {
        if (!word || !strncmp(*next_option, word, strlen(word))) {
            rc = cli_add_comphelp_entry(comphelp, *next_option);
        }
    }
    free_options(options);

    return rc;
}

