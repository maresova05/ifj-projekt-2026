/*
    IFJ a IAL projekt
    xbartlm00 Matěj Bártl
*/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "gen_base.h"
#include "dstring.h"
#include "error.h"
#include "builtins.h"


static int cycle_depth = 0;
static dynamic_string_t cycle_code_buf;
static dynamic_string_t defvar_buf;
static void gen_buf(dynamic_string_t *buf,const char *format,va_list args){
    va_list kopie;
    va_copy(kopie,args);
    int vsn = vsnprintf(NULL, 0, format, kopie);
    if (vsn < 0){
        report_error(ERR_INTERNAL,0);
    }
    va_end(kopie);
    char *pole = malloc(vsn + 1);
    if (pole == NULL){
        report_error(ERR_INTERNAL, 0);
    }
    vsnprintf(pole, vsn + 1, format, args);
    for(int idx = 0; idx < vsn;idx++){
        if(dstring_append(buf,pole[idx]) == false){
            free(pole);
            report_error(ERR_INTERNAL,0);
        }
    }
    free(pole);
}
static void gen_buf_fmt(dynamic_string_t *buf, const char *format, ...){
    va_list args;
    va_start(args, format);
    gen_buf(buf,format,args);
    va_end(args);
}
void gen_head(void){
    gen_emit(".IFJcode26\n");
    gen_emit("DEFVAR GF@$expr_op1\n");
    gen_emit("DEFVAR GF@$expr_op2\n");
}

void gen_start(void){
    gen_emit("CREATEFRAME\n");
    gen_emit("CALL %s\n",GEN_MAIN);
    gen_emit("EXIT int@0\n");
    gen_buildins();
}

void gen_emit(const char *format, ...){
    va_list args;
    va_start(args, format);
    if(cycle_depth == 0){
        vprintf(format, args);
    } else {
        gen_buf(&cycle_code_buf,format,args);
    }
    va_end(args);
}

void gen_defvar(const char *name,int scope_id){
    if(cycle_depth == 0){
        gen_emit("DEFVAR LF@%s$%d\n",name,scope_id);
    } else {
        gen_buf_fmt(&defvar_buf,"DEFVAR LF@%s$%d\n",name,scope_id);
    }
    

}

void gen_loop_begin(void){
    if(cycle_depth == 0){
        if(dstring_init(&cycle_code_buf) == false){
            report_error(ERR_INTERNAL,0);
        }
        if(dstring_init(&defvar_buf) == false){
            report_error(ERR_INTERNAL,0);
        }
    }
    cycle_depth++;
}

void gen_loop_end(void){
    if(cycle_depth == 0){
        report_error(ERR_INTERNAL,0);
    } else {
        cycle_depth--;
        if (cycle_depth == 0){
            printf("%s",defvar_buf.str);
            printf("%s",cycle_code_buf.str);
            dstring_free(&defvar_buf);
            dstring_free(&cycle_code_buf);
        }
    }
}