#include <stdio.h>
#include "gen_control.h"

static int label_counter = 0;

int gen_new_label_id(void){
    return ++label_counter;
}

void gen_if_start(int id){
    printf("POPS GF@$expr_op1\n");
    printf("JUMPIFEQ $if_else_%d GF@$expr_op1 bool@false\n",id);
}

void gen_if_else(int id){
    printf("JUMP $if_end_%d\n",id);
    printf("LABEL $if_else_%d\n",id);
}

void gen_if_end(int id){
    printf("LABEL $if_end_%d\n",id);
}

void gen_while_start(int id){
    printf("LABEL $while_start_%d\n",id);
}

void gen_while_cond(int id){
    printf("POPS GF@$expr_op1\n");
    printf("JUMPIFEQ $while_end_%d GF@$expr_op1 bool@false\n", id);
}

void gen_while_end(int id){
    printf("JUMP $while_start_%d\n",id);
    printf("LABEL $while_end_%d\n",id);
}

void gen_func_start(const char *name){
    printf("LABEL $%s\n", name);
    printf("PUSHFRAME\n");
    printf("DEFVAR LF@%%retval\n");
    printf("MOVE LF@%%retval nil@nil\n");
}

void gen_func_end(void){
    printf("POPFRAME\n");
    printf("RETURN\n");
}

void gen_func_param(const char *name, int scope_id,int index){
    printf("DEFVAR LF@%s$%d\n",name, scope_id);
    printf("MOVE LF@%s$%d LF@%%param%d\n",name,scope_id,index);
}

void gen_return_value(void){
    printf("POPS LF@%%retval\n");
    printf("POPFRAME\n");
    printf("RETURN\n");
}