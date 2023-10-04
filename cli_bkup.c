/
// Created by ali on 9/20/23.
//
#include <libcli.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <libyang/libyang.h>
#include <libyang/parser_data.h>
#include <libyang/log.h>
#include <stdio.h>
#include <stdlib.h>


#define PORT 12345

#ifdef __GNUC__
#define UNUSED(d) d __attribute__((unused))
#else
#define UNUSED(d) d
#endif

#define USERNAME "admin"
#define PASSWORD "admin"

#define GET_NEW_MODE_STR(current_mod, new_mode) \
    strcat((char*)current_mod, strcat(":", new_mode))

struct term_mode {
    int mode;
    char* mode_desc;
    struct term_mode *prev;
}*term;

struct cligen_ASM {
    uint mode;
    char mode_prompt[10];
}cligen_ASM;

struct CmdStack {
    const char * names[20];
    int top;
} cmd_stack;


void initStack() {
    cmd_stack.top = -1;
}

int isEmpty() {
    return cmd_stack.top == -1;
}

int isFull() {
    return cmd_stack.top == 20 - 1;
}

void push( const char * value) {
    if (isFull()) {
        printf("Stack overflow\n");
        return;
    }

    cmd_stack.names[++cmd_stack.top] = value;
}

const char * pop() {
    if (isEmpty()) {
        printf("Stack underflow\n");
        return NULL; // Return a sentinel value to indicate error
    }

    return cmd_stack.names[cmd_stack.top--];
}

enum {
    MODE_FRR=10,
    MODE_OSPF,
    MODE_BGP,
    MODE_BGP_NEIGHBOR,

    MODE_IPSEC,

    MODE_AAA,

    MODE_IETF_XXX,
    MODE_INTERFACE,

    MODE_E5_E5
};

int sock_listen() {
    int sockfd, new_sock;
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;
    const int TRUE = 1;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error in socket. Exiting...");
        exit(1);
    }

    if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&TRUE,sizeof(sockfd))<0){
        perror("setsockopt");
        return -1;

    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error in binding. Exiting...");
        exit(1);
    }

    // Listen for connections
    if (listen(sockfd, 10) == 0) {
        printf("Listening...\n");
    } else {
        perror("Error in listening. Exiting...");
        exit(1);
    }
    return sockfd;
}

int sock_accept(int sockfd){
    int new_sock;
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;
    addr_size = sizeof(new_addr);
    new_sock = accept(sockfd, (struct sockaddr*)&new_addr, &addr_size);
    return new_sock;
}

int push_term_mode(int mode, char* desc){
    struct term_mode *c = malloc(sizeof(struct term_mode));
    c->mode = mode;
    c->mode_desc = desc;
//    c->mode_desc = malloc(strnlen(desc,8));
//    strcpy(c->mode_desc,desc);
    c->prev = term;
    term = c;
}

int fn_frr(struct cli_def * cli, const char *cmd, char *argv[], int argc){
    cli_set_configmode(cli,MODE_FRR,"frr");
    push_term_mode(MODE_FRR,"frr");
    return CLI_OK;
}


unsigned int regular_count = 0;
unsigned int debug_regular = 0;

int regular_callback(struct cli_def *cli) {
    regular_count++;
    if (debug_regular) {
        cli_print(cli, "Regular callback - %u times so far", regular_count);
        cli_reprompt(cli);
    }
    return CLI_OK;
}
int idle_timeout(struct cli_def *cli) {
    cli_print(cli, "Custom idle timeout");
    return CLI_QUIT;
}

void pc(UNUSED(struct cli_def *cli), const char *string) {
    printf("%s\n", string);
}

int fn_exit(struct cli_def *cli, const char *cmd, char *argv[], int argc){
    struct term_mode *term_prev;
    if (term->prev == NULL){
        return cli_exit(cli,cmd,argv,argc);
    }
    term_prev = term->prev;
    free(term);
    term = term_prev;
    cli_set_configmode(cli,term->mode,term->mode_desc);

    return CLI_OK;
}


/*
 * cmd gen functions
 *
 * */

unsigned int str2mode_hash(char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return (unsigned int)hash;
}


int yang_cmd(struct cli_def * cli, const char *cmd, char *argv[], int argc){
    printf("%d\n",argc);
    if (argc == 1)
        return CLI_AMBIGUOUS;
    if ( strcmp(argv[0], "?") == 0) {
        return CLI_AMBIGUOUS;
    }
    cli_print(cli,"running cmd=%s\n",cmd);
    int mode = str2mode_hash((char*)cmd);
    cli_set_configmode(cli,mode,cmd);
    push_term_mode(mode,(char*)cmd);
    return CLI_OK;
}

int register_cmd_container(const char *name,char *parent_name,struct cli_def * cli){
    char help[100];
    sprintf(help, "configure setting for %s", name);
    unsigned int mode;
    if (parent_name == NULL)
        mode = MODE_CONFIG;
    else
        mode = str2mode_hash(parent_name);
    cli_register_command(cli,NULL, name, yang_cmd, PRIVILEGE_UNPRIVILEGED, mode , help);

}

int register_cmd_leaf(const char *name,char *parent_name,struct cli_def * cli){
    char help[100];
    sprintf(help, "configure %s", name);
    unsigned int mode;
    if (parent_name == NULL)
        mode = MODE_CONFIG;
    else
        mode = str2mode_hash(parent_name);

    cli_register_command(cli,NULL, name, yang_cmd, PRIVILEGE_UNPRIVILEGED,mode , help);
}

void register_node_cmd(struct lysc_node *schema,struct cli_def * cli){
    if (schema->flags & LYS_CONFIG_R)
        return;

    char *parent_name;
    if (schema->parent == NULL)
        parent_name= NULL;
    else
        parent_name = (char*)schema->parent->name;




    switch (schema->nodetype) {
        case LYS_CONTAINER:
            printf("CLI command for container: %s\n", schema->name);
            register_cmd_container(schema->name,parent_name,cli);
            break;
        case LYS_LEAF:
            printf("CLI command for leaf: %s\n", schema->name);
            register_cmd_leaf(schema->name,parent_name,cli);
            break;
        case LYS_LIST:
            printf("CLI command for list: %s\n", schema->name);
            register_cmd_container(schema->name,parent_name,cli);
            break;
            // Add cases for other node types as needed

        default:
            break;
    }
}


int yang_to_cmd_gen(struct lysc_node *schema,struct cli_def * cli){

    struct lysc_node *child = NULL;
    LYSC_TREE_DFS_BEGIN(schema, child)
        {
            register_node_cmd(child,cli);
        LYSC_TREE_DFS_END(schema->next,child);
    }

}


/*
 * YANG functions
 *
 * */



struct lysc_node * get_module_data(){

    int ret;
    struct ly_ctx *ctx;
    ly_log_level(LY_LLDBG);
    ret = ly_ctx_new("/home/ali/Documents/dentOS_project/yang/standard/ietf/RFC", LY_CTX_ALL_IMPLEMENTED, &ctx);
    if (ret > 0) {
        fprintf(stderr, "Failed to create libyang context: %d\n", ret);
        return NULL;
    }


    const struct lys_module *module1 = ly_ctx_load_module(ctx, "ietf-interfaces", NULL,NULL);
    const struct lys_module *module = ly_ctx_load_module(ctx, "ietf-ip", NULL, NULL);


    if (!module1) {
        fprintf(stderr, "Failed to load YANG module\n");
        ly_ctx_destroy(ctx);
        return NULL;
    }

    struct lysc_module *module_compiled = module1->compiled;
    if (module_compiled == NULL) {


        fprintf(stderr, "error: module_compiled is null\n");
        ly_ctx_destroy(ctx);
        return NULL;
    }

    struct lysc_node *node_root = module_compiled->data;
    if (node_root == NULL) {
        fprintf(stderr, "error: node_root is null\n");
        ly_ctx_destroy(ctx);
        return NULL;
    }
    return node_root;
}


/*
 * cli functions
 *
 * */


int check_auth(const char *username, const char *password) {
    if (strcasecmp(username,  USERNAME) != 0) return CLI_ERROR;
    if (strcasecmp(password, PASSWORD) != 0) return CLI_ERROR;
    return CLI_OK;
}



/* main  */


int main(){
    struct cli_def *cli= cli_init();
    char* banner ="welcome to e5-e5 cli\n";
    cli_set_banner(cli,banner);
    cli_set_hostname(cli,"dentos_r01");
    term = malloc(sizeof(struct term_mode));
    term->mode = MODE_CONFIG;
    term->prev = NULL;

    cli_set_auth_callback(cli, check_auth);


    cli_register_command(cli,NULL, "exit", fn_exit, PRIVILEGE_UNPRIVILEGED, MODE_ANY, "exit to prev mode");
    cli_register_command(cli,NULL, "frr", fn_frr, PRIVILEGE_UNPRIVILEGED, MODE_CONFIG, "frr subsystem config");

    struct lysc_node *schema = get_module_data();

    yang_to_cmd_gen(schema, cli);

    int sockfd = sock_listen();
    while (1){
        int sockfdc = sock_accept(sockfd);
        cli_loop(cli,sockfdc);

    }
    cli_done(cli);

}