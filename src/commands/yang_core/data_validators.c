//
// Created by ali on 10/22/23.
//

#include "data_validators.h"
#include <libyang/libyang.h>
#include <libyang/parser_data.h>
#include <libyang/tree.h>
#include <libyang/log.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

void to_lower(char *str) {
    while (*str) {
        *str = tolower(*str);
        str++;
    }
}

int validate_bool(struct cli_def *cli, const char *word, const char *value){
    char * value_cpy = strdup(value);

    to_lower(value_cpy);

    if ((strcmp(value,"true") !=0) && (strcmp(value,"false") !=0) ){
        cli_print(cli,"ERROR please entry ture or false\n");
        return CLI_ERROR;
    }
    return CLI_OK;

}

int validate_string(struct cli_def *cli, const char *word, const char *value,struct lysc_node * y_node){
    return CLI_OK;
}

int validate_int(struct cli_def *cli, const char *word, const char *value,struct lysc_node * y_node){
    struct lysc_node_leaf *leaf = (struct lysc_node_leaf *)y_node;
    return CLI_OK;
}

int yang_data_validator(struct cli_def *cli, const char *word, const char *value,void * cmd_model){
    struct lysc_node * y_node = (struct lysc_node *)cmd_model;
    int ret = CLI_OK;
    if (y_node->nodetype == LYS_LEAF){
        struct lysc_node_leaf *leaf = (struct lysc_node_leaf *)y_node;
        switch (leaf->type->basetype) {
            case LY_TYPE_BOOL:
                ret= validate_bool(cli,word,value);
                break;
            case LY_TYPE_STRING:
                ret = validate_string(cli,word,value,y_node);
                break;
            case LY_TYPE_UINT8:
                ret = validate_string(cli,word,value,y_node);
                break;
            default:
                ret= CLI_OK;
        }
    }
    return ret;
}