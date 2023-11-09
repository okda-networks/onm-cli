//
// Created by ali on 10/22/23.
//

#include "data_validators.h"
#include <ctype.h>
#include <string.h>

void to_lower(char *str) {
    while (*str) {
        *str = tolower(*str);
        str++;
    }
}

int is_numeric(const char *str) {
    while (*str) {
        if (!isdigit(*str)) {
            return 0; // Not numeric
        }
        str++;
    }
    return 1; // Numeric
}


int validate_all(struct cli_def *cli, const char *word, const char *value, struct lysc_node_leaf *leaf) {
    ly_err_clean(leaf->module->ctx, NULL);
    LY_ERR err = lyd_value_validate(leaf->module->ctx, (const struct lysc_node *) leaf, value,
                                    strlen(value), NULL, NULL, NULL);
    if (err == LY_SUCCESS)
        return CLI_OK;
    else {
        cli_print(cli, " ERROR: invalid value for %s, err_code=%d error=%s", word, err, ly_errmsg(leaf->module->ctx));
        return CLI_ERROR_ARG;
    }
}

int validate_uint(struct cli_def *cli, const char *word, const char *value, struct lysc_node_leaf *leaf) {

    if (!is_numeric(value)) {
        cli_print(cli, "ERROR please entry numeric value");
        return CLI_ERROR;
    }
    struct lysc_type_num *num_t = (struct lysc_type_num *) leaf->type;
    if (num_t->range == NULL) {
        cli_print(cli, "hooooooooooooon");
        return CLI_OK;
    }


    int num = atoi(value);
    int min = num_t->range->parts->min_u64;
    int max = num_t->range->parts->max_u64;

    if (min <= num && num <= max)
        return CLI_OK;
    else {
        cli_print(cli, "ERROR out of range value, min=%d , max=%d, value=%d", min, max, num);
        return CLI_ERROR_ARG;
    }


}


int yang_data_validator(struct cli_def *cli, const char *word, const char *value, void *cmd_model) {
    int ret = CLI_OK;
    struct lysc_node *y_node = (struct lysc_node *) cmd_model;
    struct lysc_node_leaf *leaf;
    // the value might be leaf value or list key leaf value, need to be handled.
    if (y_node->nodetype == LYS_LEAF)
        leaf = (struct lysc_node_leaf *) y_node;
    else if (y_node->nodetype == LYS_LIST) {
        const struct lysc_node *child_list = lysc_node_child(y_node);
        const struct lysc_node *child;
        LY_LIST_FOR(child_list, child) {
            if ((child->flags & LYS_KEY) && (strcmp(child->name, word) == 0)) {
                leaf = (struct lysc_node_leaf *) child;
                break;
            }
        }
        if (leaf == NULL) {
            cli_print(cli, "WARNING: failed to get yang_node of %s, no validation will be done", word);
            return CLI_OK;
        }

    } else if (y_node->nodetype == LYS_CASE) {
        // if choice's case we need to get its child
        const struct lysc_node *child = lysc_node_child(y_node);
        leaf = (struct lysc_node_leaf *) child;
    } else {

        return CLI_OK;
    }


    switch (leaf->type->basetype) {
        case LY_TYPE_UINT8:
        case LY_TYPE_UINT16:
        case LY_TYPE_UINT32:
        case LY_TYPE_UINT64:
            ret = validate_uint(cli, word, value, leaf);
            break;
        case LY_TYPE_INT8:
        case LY_TYPE_INT16:
        case LY_TYPE_INT32:
        case LY_TYPE_INT64:
            ret = validate_uint(cli, word, value, leaf);
            break;
        case LY_TYPE_IDENT:
//            ret = validate_ident(cli, word, value, leaf);
//            break;
        default:
            ret = validate_all(cli, word, value, leaf);
    }

    return ret;
}