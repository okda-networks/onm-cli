//
// Created by ali on 10/4/23.
//

#include "utils.h"
#include "commands.h"
#include "onm_yang.h"


#ifdef __GNUC__
#define UNUSED(d) d __attribute__((unused))
#else
#define UNUSED(d) d
#endif


/*
 * Forward Declarations for cli default commands
 * */
static int cmd_frr(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);

static int cmd_exit(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);

static int cmd_yang_generic(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);

int cmd_yang_generic_leaf(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);


static int cmd_yang2cmd_generate(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);

static int cmd_yang_path(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc);



/*
 * Globals
 * */
struct term_mode_t {
    int mode;
    char *mode_desc;
    struct term_mode_t *prev;
} *term_mode;

enum {
    MODE_FRR = 10,

};


int push_term_mode(int mode, char *desc) {
    struct term_mode_t *c = malloc(sizeof(struct term_mode_t));
    c->mode = mode;
    c->mode_desc = desc;
    c->prev = term_mode;
    term_mode = c;
}

/*
 * cli commands generator functions
 * */

/**
 * register command in cli for a container node
 * @param name
 * @param parent_name
 * @param cli
 * @return
 */
int register_cmd_container(struct lysc_node *y_node, struct cli_def *cli) {
    char help[100];
    sprintf(help, "configure setting for %s", y_node->name);
    unsigned int mode;
    if (y_node->parent == NULL)
        mode = MODE_CONFIG;
    else
        mode = str2int_hash((char *) y_node->parent->name);
    cli_register_command(cli, NULL, y_node, y_node->name, cmd_yang_generic, PRIVILEGE_UNPRIVILEGED, mode, help);

}

/**
 * register command in cli for a leaf node
 * @param name
 * @param parent_name
 * @param cli
 * @return
 */
int register_cmd_leaf(struct lysc_node *y_node, struct cli_def *cli) {
    char help[100];
    sprintf(help, "configure %s", y_node->name);
    unsigned int mode;
    if (y_node->parent == NULL)
        mode = MODE_CONFIG;
    else
        mode = str2int_hash((char *) y_node->parent->name);



    struct cli_command *c = cli_register_command(cli, NULL, y_node, y_node->name,cmd_yang_generic_leaf , PRIVILEGE_UNPRIVILEGED, mode, help);

    cli_register_optarg(c, y_node->name, CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, mode,
                        y_node->dsc, NULL, NULL, NULL);

    return 0;
}

/**
 * register cmd commands from a schema.
 * @param schema  lysc_node schema
 * @param cli     libcli cli
 * @return
 */
int register_cmds_schema(struct lysc_node *schema, struct cli_def *cli) {
    printf("DEBUG:commands.c: registering schema for  `%s`\n", schema->name);

    struct lysc_node *child = NULL;
    LYSC_TREE_DFS_BEGIN(schema, child) {
            if (child->flags & LYS_CONFIG_R) {
                return 0;
            }
            switch (child->nodetype) {
                case LYS_CONTAINER:
                    printf("TRACE: register CLI command for container: %s\n", child->name);
                    register_cmd_container(child, cli);
                    break;
                case LYS_LEAF:
                    printf("TRACE: register CLI command for leaf: %s\n", child->name);
                    register_cmd_leaf(child, cli);
                    break;
                case LYS_LIST:
                    printf("TRACE: register CLI command for list: %s\n", child->name);
                    register_cmd_container(child, cli);
                    break;
                    // Add cases for other node types as needed
                default:
                    break;
            }
        LYSC_TREE_DFS_END(schema->next, child);
    }
    printf("DEBUG:commands.c: schema `%s` registered successfully\n", schema->name);


}

/*
 * default commands definition
 * */


unsigned int regular_count = 0;
unsigned int debug_regular = 0;

int cmd_frr(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    cli_set_configmode(cli, MODE_FRR, "frr");
    push_term_mode(MODE_FRR, "frr");
    return CLI_OK;
}

int cmd_exit(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    struct term_mode_t *term_prev;
    if (term_mode->prev == NULL) {
        return cli_exit(cli, c, cmd, argv, argc);
    }
    term_prev = term_mode->prev;
    free(term_mode);
    term_mode = term_prev;
    cli_set_configmode(cli, term_mode->mode, term_mode->mode_desc);

    return CLI_OK;
}

int cmd_yang_generic_leaf(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    printf("DEBUG:commands.c:cmd_yang_generic(): executing command `%s` , help=%s\n", cmd, (char *) cli->user_context);
    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            return CLI_INCOMPLETE_COMMAND;
        } else {
            cli_print(cli, "command exuected succesfful, cmd=%s %s\n",cmd,argv[0]);
            return CLI_OK;
        }


    }
    return CLI_MISSING_ARGUMENT;

}

int cmd_yang_generic(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    printf("DEBUG:commands.c:cmd_yang_generic(): executing command `%s` , help=%s\n", cmd, (char *) cli->user_context);
    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            return CLI_INCOMPLETE_COMMAND;
        }
        cli_print(cli, "unknown args\n");
        return CLI_ERROR_ARG;
    }

    struct lysc_node *ne = (struct lysc_node *) c->cmd_model;
    char xpath[100];

    lysc_path(ne,LYSC_PATH_DATA,xpath,100);


    if (ne != NULL)
        cli_print(cli, "this command is for module=%s , node=%s, xpath=%s", ne->module->name, ne->name,xpath);
    else
        cli_print(cli, "failed to fine yang module");

    int mode = str2int_hash((char *) cmd);
    cli_set_configmode(cli, mode, cmd);
    push_term_mode(mode, (char *) cmd);
    return CLI_OK;
}

int cmd_yang2cmd_generate(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    printf("DEBUG:commands.c:cmd_yang2cmd_generate(): executing command `%s`\n", cmd);

    if (argc == 0) {
        cli_print(cli, "please specify yang module name");
        return CLI_INCOMPLETE_COMMAND;
    }
    if (strcmp(argv[0], "?") == 0) {
        cli_print(cli, "specify yang module name to generate commands from");
        return CLI_OK;
    }
    char *module_name = (char *) argv[0];
    struct lysc_node *schema = get_module_schema(module_name);
    if (schema == NULL) {
        cli_print(cli, "ERROR: yang module '%s' not found, "
                       "please make sure to set the correct search dir for yang, use command 'yang search_dir set /path/to/yang_modules' in enable mode"
                       "", module_name);
        return CLI_ERROR;
    }
    cli_print(cli, "generating commands for yang module `%s`", module_name);
    register_cmds_schema(schema, cli);
    cli_print(cli, "commands for yang module `%s` generated successfully", module_name);
    return CLI_OK;

}

int cmd_yang_searchdir_set(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (argc == 0) {
        cli_print(cli, "specify yang dir");
        return CLI_INCOMPLETE_COMMAND;
    }
    if (strcmp(argv[0], "?") == 0) {
        cli_print(cli, "yang dir was not specified");
        return CLI_MISSING_ARGUMENT;
    }

    set_yang_searchdir(argv[0]);
    return CLI_OK;

}

int cmd_yang_searchdir_unset(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (argc == 0) {
        cli_print(cli, "specify yang dir");
        return CLI_INCOMPLETE_COMMAND;
    }
    if (strcmp(argv[0], "?") == 0) {
        cli_print(cli, "yang dir was not specified");
        return CLI_MISSING_ARGUMENT;
    }

    unset_yang_searchdir(argv[0]);
    return CLI_OK;

}

int cmd_yang_list_searchdirs(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, "<cr>");
            return CLI_MISSING_ARGUMENT;
        }
    }

    const char *const *dir_list = get_yang_searchdirs();
    while (*dir_list != NULL) {
        cli_print(cli, "[+] %s", *dir_list);
        ++dir_list;
    }


}

int cmd_regular_callback(struct cli_def *cli) {
    regular_count++;
    if (debug_regular) {
        cli_print(cli, "Regular callback - %u times so far", regular_count);
        cli_reprompt(cli);
    }
    return CLI_OK;
}


int check_auth(const char *username, const char *password) {
    if (strcasecmp(username, USERNAME) != 0) return CLI_ERROR;
    if (strcasecmp(password, PASSWORD) != 0) return CLI_ERROR;
    return CLI_OK;
}


int onm_commands_init(struct cli_def *cli) {
    printf("INFO:commands.c: initializing commands\n");



    // init term mode
    term_mode = malloc(sizeof(struct term_mode_t));
    term_mode->mode = MODE_CONFIG;
    term_mode->prev = NULL;

    cli_set_auth_callback(cli, check_auth);
    cli_register_command(cli, NULL, NULL, "exit",
                         cmd_exit, PRIVILEGE_UNPRIVILEGED,
                         MODE_ANY, "exit to prev mode");
    cli_register_command(cli, NULL, NULL,
                         "frr", cmd_frr, PRIVILEGE_UNPRIVILEGED,
                         MODE_CONFIG, "frr subsystem config");
    // yang commands
    struct cli_command *yang_cmd = cli_register_command(cli, NULL, NULL,
            "yang", NULL, PRIVILEGE_PRIVILEGED,
            MODE_EXEC,"yang settings");
    if (yang_cmd == NULL)
        printf("failed\n");
    struct cli_command *yang_set_cmd = cli_register_command(cli, yang_cmd, NULL,
            "set", NULL, PRIVILEGE_UNPRIVILEGED,
            MODE_EXEC, "set yang settings");
    struct cli_command *yang_unset_cmd = cli_register_command(cli, yang_cmd, NULL,
            "unset", NULL,PRIVILEGE_UNPRIVILEGED,
            MODE_EXEC, "unset yang settings");

    cli_register_command(cli, yang_set_cmd, NULL,
                         "searchdir", cmd_yang_searchdir_set, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC,"set yang search dir: yang set searchdir <path/to/modules>");
    cli_register_command(cli, yang_unset_cmd, NULL,
                         "searchdir", cmd_yang_searchdir_unset, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, "unset yang search dir: yang set searchdir <path/to/modules>");

    cli_register_command(cli, yang_cmd, NULL,
                         "generate-commands", cmd_yang2cmd_generate, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, "generate commands from module:yang generate-commands <module-name>");
    cli_register_command(cli, yang_cmd, NULL,
                         "list-seachdir", cmd_yang_list_searchdirs, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, "list yang serachdirs");

}
