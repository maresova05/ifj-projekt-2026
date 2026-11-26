/*
    IFJ a IAL projekt
    xbartlm00 Matěj Bártl
*/
#include "builtins.h"
#include "gen_base.h"
#include "error.h"

void gen_operand(Token *token,SymbolData *sym){
    switch(token->type){
    case (TOKEN_INT_LITERAL):
        gen_emit("int@%lld",token->attribute.i_val);
        break;
    case (TOKEN_DOUBLE_LITERAL):
        gen_emit("float@%a",token->attribute.d_val);
        break;
    case (TOKEN_STRING_LITERAL):
        gen_emit("string@");
        const char *s = token->attribute.s_val;
        for (size_t i = 0; s[i] != '\0'; i++) {
            unsigned char c = (unsigned char)s[i];
            if(c <= 32 || c == 35 || c == 92){
                gen_emit("\\%03d",c);
            } else {
                gen_emit("%c",c);
            }
        }
        break;
    case (TOKEN_IDENTIFIER):
        if(sym != NULL){
            gen_emit("LF@%s$%d",token->attribute.s_val,sym->scope_id);
        } else {
            report_error(ERR_INTERNAL,0);
        }
        break;
    case (TOKEN_TRUE):
        gen_emit("bool@true");
        break;
    case (TOKEN_FALSE):
        gen_emit("bool@false");
        break;
    default:
        report_error(ERR_INTERNAL,0);
    }
}

void gen_print(Token *token,SymbolData *sym){
    if(sym != NULL && sym->data_type == TYPE_UNIT){
        gen_emit("WRITE string@()\n");
    } else {
        gen_emit("WRITE ");
        gen_operand(token, sym);
        gen_emit("\n");
    }
}