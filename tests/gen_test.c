
#include <stdio.h>
#include "../gen_base.h"

int main(void){
    gen_emit("LABEL $$pred\n");
    gen_loop_begin();
    gen_emit("LABEL $$loop\n");
    gen_defvar("x", 3);
    gen_loop_begin();
    gen_defvar("y", 4);
    gen_loop_end();
    gen_emit("JUMP $$loop\n");
    gen_loop_end();
    gen_emit("LABEL $$po\n");
    return 0;
}