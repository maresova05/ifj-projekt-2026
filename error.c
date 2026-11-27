/*
    IFJ a IAL projekt
    xbartlm00 Matěj Bártl
*/

#include <stdio.h>
#include <stdlib.h>
#include "error.h"

static const char *error_to_string(Error_code err){
    switch(err){
        case(ERR_LEXICAL):
            return "Lexical error";
        case(ERR_SYNTAX):
            return "Syntax error";
        case(ERR_UNDEFINED_SYMBOL):
            return "Undefined symbol";
        case(ERR_REDEFINITION):
            return "Redefinition";
        case(ERR_ARGUMENTS):
            return "Invalid arguments";
        case(ERR_TYPE):
            return "Type error";
        case(ERR_IMMUTABLE):
            return "Immutable variable error";
        case(ERR_OTHER_SEMANTIC):
            return "Semantic error";
        case(ERR_INTERNAL):
            return "Internal error";
        default:
            return NULL;
    }
}

void report_error(Error_code err,int line){
    if(error_to_string(err) == NULL){
        fprintf(stderr,"Internal error: unknown error code\n");
        exit(ERR_INTERNAL);
    } else {
        if(line < 1){
        fprintf(stderr,"%s\n",error_to_string(err));
        } else {
        fprintf(stderr,"%s on line %d\n",error_to_string(err),line);
        }
        exit(err);
    }
    
}
    
