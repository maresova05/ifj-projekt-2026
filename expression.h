#ifndef EXPRESSION_H
#define EXPRESSION_H

typedef enum {
    EXPR_OK = 0,            //Vše ok        
    EXPR_ERR_SYNTAX = 2,    //Syntaktická chyba        
    EXPR_ERR_SEMANTC = 5,   //Sémantická chyba
    EXPR_ERR_INTERNAL = 99  //Ostatní chyby (selhání malloc, ...)
}ExprResult;

/**
 * Hlavní funkce pro syntaktickou analýzu výrazu.
 * @return ExprResult Vrací EXPR_OK(0) při úspěchu, nebo příslušný chybový kód.
 */
ExprResult parse_expression();

#endif //EXPRESSION_H