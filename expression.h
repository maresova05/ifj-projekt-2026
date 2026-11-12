typedef enum {
    EXPR_OK = 0,            //Vše ok        
    EXPR_ERR_SYNTAX = 2,    //Syntaktická chyba        
    EXPR_ERR_SEMANTC = 5,   //Sémantická chyba
    EXPR_ERR_INTERNAL = 99  //Ostatní chyby (selhání malloc, ...)
}ExprResult;