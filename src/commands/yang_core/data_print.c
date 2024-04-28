#include <stdlib.h>
#include <bsd/string.h>
#include "data_print.h"

void print_indentation(int pos, char **result) {
    if (pos > 0) {
        ssize_t current_size = strlen(*result) + 1;
        ssize_t new_size = current_size + pos;
        *result = realloc(*result, current_size + new_size);
        memset(*result + current_size, 0, new_size);
        // add indent
        for (int i = 0; i < pos; i++) {
            strlcat(*result, " ", current_size + new_size);
        }
    }
}

void print_root_container_dnode(struct lyd_node *dnode, int pos, char **result) {
    char line[1024] = {0};
    strlcat(line, dnode->schema->name, sizeof(line));
    strlcat(line, "\n", sizeof(line));

    size_t new_size = strlen(line) + 1; // Length of schema name + null terminator
    // Calculate the new size, including the existing content and the new schema name
    size_t current_size = strlen(*result);

    *result = realloc(*result, current_size + new_size);
    memset(*result + current_size, 0, new_size);

    strlcat(*result, line, current_size + new_size);
}

// this print parents on the
void print_inline_parents_dnode(const struct lysc_node *snode, int pos, char **result) {
    // first check if node has parent container or choice which should be printed on same line.
    struct lysc_node *parent_snode = snode->parent;
    if (parent_snode) {
        if (parent_snode->nodetype != LYS_LIST)
            print_inline_parents_dnode(parent_snode, pos, result);
        else
            print_indentation(pos, result);

    } else
        return;

    char line[1024] = {0};
    // special case for choice which happen to have duplicate child containers. example tc-filter will print:
    // "filter flower flower action drop" is this if statement is not added
    if (strcmp(parent_snode->name, snode->name)) {
        strlcat(line, snode->name, sizeof(line));
        strlcat(line, " ", sizeof(line));
    }

    size_t new_size = strlen(line) + 1; // Length of schema name + null terminator
    // Calculate the new size, including the existing content and the new schema name
    size_t current_size = strlen(*result);

    *result = realloc(*result, current_size + new_size);
    memset(*result + current_size, 0, new_size);

    strlcat(*result, line, current_size + new_size);
}


void print_list_dnode(struct lyd_node *dnode, int pos, char **result) {
    print_indentation(pos, result);
    char line[1024] = {0};
    strlcat(line, dnode->schema->name, sizeof(line));
    strlcat(line, " ", sizeof(line));
    struct lyd_node *next, *childs = lyd_child(dnode);
    LY_LIST_FOR(childs, next)
    {
        if (lysc_is_key(next->schema)) {
            strlcat(line, lyd_get_value(next), sizeof(line));
            strlcat(line, " ", sizeof(line));
        }
    }
    strlcat(line, "\n", sizeof(line));

    size_t new_size = strlen(line) + 1; // Length of line + null terminator
    size_t current_size = strlen(*result);
    *result = realloc(*result, current_size + new_size);
    memset(*result + current_size, 0, new_size); // Initialize newly allocated portion to zeros


    strlcat(*result, line, current_size + new_size);
}

void print_leaf_dnode(struct lyd_node *dnode, int pos, char **result) {
    // key leaf is already printed by print_list_dnode()
    if (lysc_is_key(dnode->schema)) {
        return;
    }
    struct lyd_node *parent_node;
    parent_node = lyd_parent(dnode);
    // if the parent node is container this add it to the line.
    if (parent_node->schema->nodetype == LYS_CONTAINER) {
        print_inline_parents_dnode(parent_node->schema, pos, result);
    } else {
        print_indentation(pos, result);
    }
    char line[1024] = {0};
    strlcat(line, dnode->schema->name, sizeof(line));
    strlcat(line, " ", sizeof(line));
    strlcat(line, lyd_get_value(dnode), sizeof(line));
    strlcat(line, "\n", sizeof(line));

    size_t new_size = strlen(line) + 1; // Length of line + null terminator

    size_t current_size = strlen(*result);
    *result = realloc(*result, current_size + new_size);
    memset(*result + current_size, 0, new_size); // Initialize newly allocated portion to zeros


    strlcat(*result, line, current_size + new_size);
}

void print_leaflist_dnode(struct lyd_node *dnode, int pos, char **result) {
    // TODO: leaflist handler
    print_leaf_dnode(dnode, pos, result);
}


void core_config_print(struct lyd_node *d_node, int pos, char **result) {

    switch (d_node->schema->nodetype) {
        case LYS_LIST:
            print_list_dnode(d_node, pos, result);
            break;
        case LYS_CONTAINER:
            if (lyd_parent(d_node) == NULL)
                print_root_container_dnode(d_node, pos, result);
            break;
        case LYS_LEAF:
            print_leaf_dnode(d_node, pos, result);
            break;
        case LYS_LEAFLIST:
            print_leaflist_dnode(d_node, pos, result);
            break;
    }
    struct lyd_node *child_list = lyd_child(d_node);
    if (child_list) {
        struct lyd_node *next;
        LY_LIST_FOR(child_list, next)
        {
            if (d_node->schema->nodetype == LYS_CONTAINER &&
                lyd_parent(d_node) != NULL) // we have the container and it children on same line
                core_config_print(next, pos, result);
            else
                core_config_print(next, pos + 2, result);
        }
    }
}

void config_print_mem(char **result, struct lyd_node *d_node) {
    *result = malloc(2);  // Initialize result
    memset(*result, 0, 2);
    core_config_print(d_node, 0, result);
}
