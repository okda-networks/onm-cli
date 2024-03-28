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

#include <libyang/parser_schema.h>
#include "src/onm_logger.h"
#include "src/onm_sysrepo.h"
#include "data_factory.h"
#include "data_cmd_compl.h"


void identityref_add_comphelp(struct lysc_ident *identity, const char *word, struct cli_comphelp *comphelp) {
    if (!identity) {
        return;
    }
    char **id_entry;
    LY_ARRAY_COUNT_TYPE i;
    LY_ARRAY_FOR(identity->derived, i)
    {
        char *id_str = malloc(strlen(identity->derived[i]->name) + strlen(identity->derived[i]->module->name) + 2);
        sprintf(id_str, "%s:%s", identity->derived[i]->module->name, identity->derived[i]->name);

        id_entry = &id_str;
        if (!word || !strncmp(*id_entry, word, strlen(word))) {
            cli_add_comphelp_entry(comphelp, *id_entry);
        }
        free(id_str);
        identityref_add_comphelp(identity->derived[i], word, comphelp);
    }
    return;
}


void free_options(const char **options) {
    if (options) {
        for (int i = 0; options[i] != NULL; i++) {
            free((void *) options[i]);  // Correctly cast to void* for freeing
        }
        free(options);
    }
}

enum {
    CANDIDATE_SRC,
    RUNNING_SRC,
    STARTUP_SRC,
    CANDIDATE_OR_RUNNING_SRC,
};

const char **get_list_key_values_array(struct lysc_node *y_node, int num_args, int ds) {
    const char **env_vars = NULL;
    struct lyd_node *list_data_node = NULL;
    char xpath[1024] = {0};

    switch (ds) {
        case CANDIDATE_SRC:
            list_data_node = get_local_list_nodes(y_node->parent);
            break;
        case CANDIDATE_OR_RUNNING_SRC: // for show config, if no data in candidate check startup and running
            list_data_node = get_local_or_sr_list_nodes(y_node->parent);
            if (list_data_node == NULL) {
                if (y_node->parent != NULL && y_node->parent->parent != NULL)
                    list_data_node = get_sysrepo_running_node(xpath);
            }
            break;
            // running and startup auto complete will be only for show commands, so we can use xpath as we
            // don't show beyond first two nodes.
        case RUNNING_SRC:
            if (y_node->parent != NULL && y_node->parent->parent != NULL) {
                lysc_path(y_node->parent->parent, LYSC_PATH_DATA, xpath, 1028);
                list_data_node = lyd_child(get_sysrepo_running_node(xpath));
            }
            break;
        case STARTUP_SRC:
            if (y_node->parent != NULL && y_node->parent->parent != NULL) {
                lysc_path(y_node->parent->parent, LYSC_PATH_DATA, xpath, 1028);
                list_data_node = lyd_child(get_sysrepo_startup_node(xpath));
            }
            break;
    }
    struct lyd_node *next = NULL;
    LY_LIST_FOR(list_data_node, next)
    {
        struct lyd_node *entry_children = lyd_child(next);
        struct lyd_node *entry_child = NULL;
        LY_LIST_FOR(entry_children, entry_child)
        {
            if (lysc_is_key(entry_child->schema) && !strcmp(entry_child->schema->name, y_node->name)
                && !strcmp(next->schema->name, y_node->parent->name)) {
                num_args++;
                env_vars = realloc(env_vars, sizeof(env_vars) * num_args);
                env_vars[num_args - 1] = strdup(lyd_get_value(entry_child));
            }
        }
    }
    if (env_vars != NULL) {
        env_vars = realloc(env_vars, sizeof(env_vars) * (num_args + 1));
        env_vars[num_args] = NULL;
    }
    return env_vars;
}

/**
 * @brief check if str exist in options (used to avoid duplications)
 * @param options options strings
 * @param str string to check if exist in options
 * @param size size of options
 * @return 1 if exist
 * @return 0 if does not exist
 */
int options_contain(const char **options,const char *str, int size) {
    for (int i = 0; i < size; i++) {
        if (strcmp(str, options[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

const char **create_type_options(struct lysc_node *y_node, int datastore, struct lysc_type *in_y_node_type) {

    if (y_node == NULL)
        return NULL;

    int num_args = 0;
    const char **env_vars = NULL;
    LY_ARRAY_COUNT_TYPE i_sized;
    struct lysc_type *y_node_type;

    // this will hanlde the union case, as in union we want to use multiple types for same node,
    // union case will call create_type_options for each type inside the union.
    if (in_y_node_type == NULL)
        y_node_type = (struct lysc_type *) ((struct lysc_node_leaf *) y_node)->type;
    else
        y_node_type = in_y_node_type;
    LY_DATA_TYPE type = y_node_type->basetype;
    switch (type) {
        case LY_TYPE_ENUM: {
            struct lysc_type_enum *y_enum_type = (struct lysc_type_enum *) y_node_type;
            // Populate the array with enum names
            LY_ARRAY_FOR(y_enum_type->enums, i_sized)
            {
                num_args++;
                env_vars = realloc(env_vars, sizeof(env_vars) * num_args);
                env_vars[num_args - 1] = strdup(y_enum_type->enums[i_sized].name);
            }
        }
            break;

        case LY_TYPE_BOOL:
            env_vars = malloc(3 * sizeof(const char *));
            env_vars[0] = strdup("false");
            env_vars[1] = strdup("true");
            env_vars[2] = NULL;
            return env_vars;

        case LY_TYPE_LEAFREF: {
            struct lysc_node *target_node = (struct lysc_node *) lysc_node_lref_target(y_node);
            return get_list_key_values_array(target_node, num_args, CANDIDATE_OR_RUNNING_SRC);
        }

        case LY_TYPE_UNION: {
            struct lysc_type_union *y_union_type = (struct lysc_type_union *) y_node_type;


            LY_ARRAY_FOR(y_union_type->types, i_sized)
            {
                const char **tmp_env_vars;
                if (y_union_type->types[i_sized]->basetype != LY_TYPE_LEAFREF) {
                    tmp_env_vars = create_type_options(y_node, datastore,
                                                       (struct lysc_type *) y_union_type->types[i_sized]);
                    if (tmp_env_vars != NULL) {
                        for (int i = 0; tmp_env_vars[i] != NULL; i++) {
                            // check if the option already exist in the collected optoins.
                            if (options_contain(env_vars, tmp_env_vars[i],num_args) == 0) {
                                num_args++;
                                env_vars = realloc(env_vars, sizeof(env_vars) * num_args);
                                env_vars[num_args - 1] = strdup(tmp_env_vars[i]);
                            }
                        }
                        free_options(tmp_env_vars);
                    }
                } else {
                    struct ly_set *s_set;
                    struct lysc_node *target_node = NULL;
                    struct lysc_type_leafref *lref_t = (struct lysc_type_leafref *) y_union_type->types[i_sized];

                    int ret = lys_find_expr_atoms(y_node, y_node->module, lref_t->path,
                                                  lref_t->prefixes, 0, &s_set);
                    if (s_set == NULL || ret != LY_SUCCESS) {
                        LOG_ERROR("%s: failed to get target leafref for node \"%s\": %s\n",
                                  __func__, y_node->name, ly_strerrcode(ret));
                        return NULL;
                    }
                    target_node = s_set->snodes[s_set->count - 2];
                    tmp_env_vars = create_type_options((struct lysc_node *) lysc_node_child(target_node), datastore,
                                                       NULL);
                    if (tmp_env_vars != NULL) {
                        for (int i = 0; tmp_env_vars[i] != NULL; i++) {
                            num_args++;
                            env_vars = realloc(env_vars, sizeof(env_vars) * num_args);
                            env_vars[num_args - 1] = strdup(tmp_env_vars[i]);
                        }
                        free_options(tmp_env_vars);
                    }
                }
            }
            break;
        }

        default:
            // check if node is list key, then get the available keys from the ds.
            if (!lysc_is_key(y_node))
                break;
            return get_list_key_values_array(y_node, num_args, datastore);
    }
    if (env_vars != NULL) {
        env_vars = realloc(env_vars, sizeof(env_vars) * (num_args + 1));
        env_vars[num_args] = NULL;
    }

    return env_vars;

}

int core_optagr_get_compl(const char *word, struct cli_comphelp *comphelp,
                          void *cmd_model, int datastore) {
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
        LY_ARRAY_FOR(y_id_type->bases, i)
        {
            identityref_add_comphelp(y_id_type->bases[i], word, comphelp);
        }
        return CLI_OK;
    }
    options = (const char **) create_type_options(y_node, datastore,NULL);
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

int optagr_get_compl_candidate(struct cli_def *cli, const char *name, const char *word, struct cli_comphelp *comphelp,
                               void *cmd_model) {
    return core_optagr_get_compl(word, comphelp, cmd_model, CANDIDATE_SRC);
}

int optagr_get_compl_running(struct cli_def *cli, const char *name, const char *word, struct cli_comphelp *comphelp,
                             void *cmd_model) {
    return core_optagr_get_compl(word, comphelp, cmd_model, RUNNING_SRC);
}

int optagr_get_compl_startup(struct cli_def *cli, const char *name, const char *word, struct cli_comphelp *comphelp,
                             void *cmd_model) {
    return core_optagr_get_compl(word, comphelp, cmd_model, STARTUP_SRC);
}

int optagr_get_compl_candidate_running(struct cli_def *cli, const char *name, const char *word,
                                       struct cli_comphelp *comphelp,
                                       void *cmd_model) {
    return core_optagr_get_compl(word, comphelp, cmd_model, CANDIDATE_OR_RUNNING_SRC);
}

