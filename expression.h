#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "symtable.h"
#include "scanner.h"

typedef enum {
    EXPR_OK = 0,            //Vše ok        
    EXPR_ERR_SYNTAX = 2,    //Syntaktická chyba        
    EXPR_ERR_SEMANTC = 6,   //Sémantická chyba
    EXPR_ERR_INTERNAL = 99  //Ostatní chyby (selhání malloc, ...)
}ExprResult;

/**
 * Hlavní funkce pro syntaktickou analýzu výrazu.
 * @param global_table Ukazatel na globální tabulku symbolů pro kontrolu deklarací a typů.
 * @param final_type Výstupní parametr, zde se uloží výsledný datový typ výrazu
 * @param next_token Výstupní parametr, zde se uloží token na kterém se parser zastavil
 * @return ExprResult Vrací EXPR_OK(0) při úspěchu, nebo příslušný chybový kód.
 */
ExprResult parse_expression(SymTable *global_table, DataType *final_type, Token*next_token);



#endif //EXPRESSION_H