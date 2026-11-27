/*
    IFJ a IAL projekt
    xbartlm00 Matěj Bártl
*/
#include "builtins.h"
#include "gen_base.h"

void gen_buildins(void){
    gen_emit("LABEL $$print\n");
    gen_emit("PUSHFRAME\n");
    gen_emit("DEFVAR LF@%%retval\n");
    gen_emit("MOVE LF@%%retval nil@nil\n");
    gen_emit("DEFVAR LF@$type\n");
    gen_emit("TYPE LF@$type LF@%%param0\n");
    gen_emit("JUMPIFEQ $$print_unit LF@$type string@nil\n");
    gen_emit("WRITE LF@%%param0\n");
    gen_emit("JUMP $$print_end\n");
    gen_emit("LABEL $$print_unit\n");
    gen_emit("WRITE string@()\n");
    gen_emit("LABEL $$print_end\n");
    gen_emit("POPFRAME\n");
    gen_emit("RETURN\n");
}
