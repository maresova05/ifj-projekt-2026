#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "symtable.h"
#include "scanner.h"

typedef enum {
    EXPR_OK = 0,            //Vše ok        
    EXPR_ERR_LEX = 1,       //Lexikální chyba
    EXPR_ERR_SYNTAX = 2,    //Syntaktická chyba
    EXPR_ERR_UNDEF = 3,    //Nedefinovaná proměnná        
    EXPR_ERR_PARAM = 5,     //Špatný počet/typ argumentů
    EXPR_ERR_SEMANTIC = 6,   //Sémantická chyba
    EXPR_ERR_INTERNAL = 99  //Ostatní chyby (selhání malloc, ...)
}ExprResult;

/**
 * Hlavní funkce pro syntaktickou analýzu výrazu.
 * @param local_stack Ukazatel na lokální tabulku symbolů
 * @param final_type Výstupní parametr, zde se uloží výsledný datový typ výrazu
 * @param next_token Výstupní parametr, zde se uloží token na kterém se parser zastavil
 * @return ExprResult Vrací EXPR_OK(0) při úspěchu, nebo příslušný chybový kód.
 */
ExprResult parse_expression(const SymTableStack *local_stack, DataType *final_type, Token*next_token);



#endif //EXPRESSION_H