
#include <stdio.h>
#include "../builtins.h"
#include "../gen_base.h"

int main(void){
    gen_head();
    Token t1 = {0};
    t1.type = TOKEN_INT_LITERAL;
    t1.attribute.i_val = 42;
    gen_print(&t1, NULL);

    Token t2 = {0};
    t2.type = TOKEN_STRING_LITERAL;
    t2.attribute.s_val = "a b#c\\d";
    gen_print(&t2, NULL);

    Token t3 = {0};
    SymbolData sym = {0};
    sym.data_type = TYPE_UNIT;
    t3.type = TOKEN_IDENTIFIER;
    t3.attribute.s_val = "u";
    gen_print(&t3, &sym);

    return 0;
}