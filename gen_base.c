/*
    IFJ a IAL projekt
    xbartlm00 Matěj Bártl
*/

#include <stdio.h>
#include "gen_base.h"

void gen_head(void){
    printf(".IFJcode26\n");
    printf("DEFVAR GF@$expr_op1\n");
    printf("DEFVAR GF@$expr_op2\n");
}

void gen_start(void){
    printf("CREATEFRAME\n");
    printf("CALL %s\n",GEN_MAIN);
    printf("EXIT int@0\n");
}