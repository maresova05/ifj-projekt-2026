/**
 * @file gen_control.c
 * @brief Generování kódu IFJcode26 pro if,while a definice funkcí
 *
 * Projekt: Implementace překladače imperativního jazyka IFJ26
 *
 * @author Jan Dobrovolný (xdobroj00)
 */

#include <stdio.h>
#include "gen_control.h"
#include "gen_base.h"

static int label_counter = 0;

int gen_new_label_id(void){
    return ++label_counter;
}

void gen_main_label(void){
    gen_emit("LABEL %s\n",GEN_MAIN);
}

void gen_if_start(int id){
    gen_emit("POPS GF@$expr_op1\n");
    gen_emit("JUMPIFEQ $$if_else_%d GF@$expr_op1 bool@false\n",id);
}

void gen_if_else(int id){
    gen_emit("JUMP $$if_end_%d\n",id);
    gen_emit("LABEL $$if_else_%d\n",id);
}

void gen_if_end(int id){
    gen_emit("LABEL $$if_end_%d\n",id);
}

void gen_while_start(int id){
    gen_loop_begin();
    gen_emit("LABEL $$while_start_%d\n",id);
}

void gen_while_cond(int id){
    gen_emit("POPS GF@$expr_op1\n");
    gen_emit("JUMPIFEQ $$while_end_%d GF@$expr_op1 bool@false\n", id);
}

void gen_while_end(int id){
    gen_emit("JUMP $$while_start_%d\n",id);
    gen_emit("LABEL $$while_end_%d\n",id);
    gen_loop_end();
}

void gen_func_start(const char *name){
    gen_emit("LABEL $%s\n", name);
    gen_emit("PUSHFRAME\n");
    gen_emit("DEFVAR LF@%%retval\n");
    gen_emit("MOVE LF@%%retval nil@nil\n");
}

void gen_func_end(void){
    gen_emit("POPFRAME\n");
    gen_emit("RETURN\n");
}

void gen_func_param(const char *name, int scope_id,int index){
    gen_emit("DEFVAR LF@%s$%d\n",name, scope_id);
    gen_emit("MOVE LF@%s$%d LF@%%param%d\n",name,scope_id,index);
}

void gen_return_value(void){
    gen_emit("POPS LF@%%retval\n");
    gen_emit("POPFRAME\n");
    gen_emit("RETURN\n");
}

void gen_expr_discard(void){
    gen_emit("POPS GF@$expr_op1\n");
}

void gen_expr_to_retval(void){
    gen_emit("POPS LF@%%retval\n");
}