//
// Created by ali on 11/9/23.
//

#include "yang_core.h"
#include "src/onm_logger.h"

extern struct ly_ctx *yang_ctx;

enum register_node_routine_signals {
    REG_NO_SIG = 0,
    REG_SKIP_NEXT_SIG,  // skip the next node
    REG_NODE_UnKNOWN_SIG, // node unknown was not registered
    REG_OK_SIG
};

static int register_node_routine(struct cli_def *cli, struct lysc_node *schema) {
    if (schema->flags & LYS_CONFIG_R) {
        LOG_DEBUG("cmd_generator: skipping none config node: %s", schema->name);
        return REG_NO_SIG;
    }
    switch (schema->nodetype) {
        case LYS_CONTAINER:
            LOG_DEBUG("register CLI command for container: %s", schema->name);
            register_cmd_container(cli, schema);
            break;
        case LYS_LEAF:
            LOG_DEBUG("register CLI command for leaf: %s", schema->name);
            register_cmd_leaf(cli, schema);
            break;
        case LYS_LIST:
            LOG_DEBUG("register CLI command for list: %s", schema->name);
            register_cmd_list(cli, schema);
            break;
        case LYS_LEAFLIST:
            LOG_DEBUG("register CLI command for leaf-list: %s", schema->name);
            register_cmd_leaf_list(cli, schema);
            break;
        case LYS_CHOICE:
            LOG_DEBUG("register CLI command for choice: %s", schema->name);
            register_cmd_choice(cli, schema);
            return REG_SKIP_NEXT_SIG;
        default:
            return REG_NODE_UnKNOWN_SIG;
    }
    return REG_OK_SIG;
}

/**
 * register  commands from a yang schema.
 * @param schema  lysc_node schema the root node
 * @param cli     libcli cli
 * @return
 */
int register_commands_schema(struct lysc_node *schema, struct cli_def *cli) {

    struct lysc_node *child = NULL;
    int signal;
    LYSC_TREE_DFS_BEGIN(schema, child) {
            signal = register_node_routine(cli, child);
            if (signal == REG_SKIP_NEXT_SIG)
                LYSC_TREE_DFS_continue = 1;
        LYSC_TREE_DFS_END(schema, child);
    }

}

static void unregister_node_routine(struct cli_def *cli, struct lysc_node *y_node) {
    if (y_node->flags & LYS_CONFIG_R) {
        return;
    }
    // we add "print-order" command for userordered node, we need to unregister.
    if (lysc_is_userordered(y_node)){
        cli_unregister_command(cli, "print-order", "print-order");
    }
    const struct lys_module *y_owner_module = lysc_owner_module(y_node);
    cli_unregister_command(cli, y_node->name, (char *) y_owner_module->name);
}

int unregister_commands_schema(struct lysc_node *schema, struct cli_def *cli) {

    struct lysc_node *child = NULL;

    LYSC_TREE_DFS_BEGIN(schema, child) {
            LOG_DEBUG("commands.c: unregistering command for %s", child->name);
            unregister_node_routine(cli, child);
        LYSC_TREE_DFS_END(schema->next, child);
    }
    LOG_DEBUG("commands.c: schema `%s` unregistered successfully", schema->name);

}