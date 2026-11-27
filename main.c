/*
    IFJ a IAL projekt 2026
    xbartlm00 Matěj Bártl
*/

#include <stdio.h>
#include "parser.h"
#include "gen_base.h"
#include "input.h"

int main(){
    input_load();
    gen_head();
    gen_start(); 
    parse_program();
    input_free();    
    return 0;
}