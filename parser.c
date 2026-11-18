#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "scanner.h"

static Token current; // aktuální token

/**
* @brief ohlásí syntaktickou chybu a ukončí překlad
*/
static void syntax_error(const char *msg){
    fprintf(stderr,"Syntax error on line %d: %s\n",current.line,msg);
    exit(2);
}

/**
* @brief načte od lexeru další token 
*/
static void next(void){
    get_next_token(&current);
}

/**
* @brief ověří očekávaný typ tokenu a přesune se na další
* @param type očekávaný typ tokenu
* @param msg zpráva pro stderr
*/
static void expect(TokenType type, const char *msg){
    if (current.type != type){
        syntax_error(msg);
    }
    next();
}

/**
* @brief parsuje typ (pravidlo 12)
*/
static void parse_type(void){
    expect(TOKEN_IDENTIFIER,"expected identifier");
}


/**
* @brief parsuje term (pravidla 55-58)
*/
static void parse_term(void){
    switch (current.type){
        case TOKEN_IDENTIFIER:
        case TOKEN_INT_LITERAL:
        case TOKEN_DOUBLE_LITERAL:
        case TOKEN_STRING_LITERAL:
            next();
            break;
        default:
            syntax_error("expected identifier or literal");
    }
}

/**
* @brief syntaktická analýza programu rekurzivním sestupem
* @return vrací 0 při úspěchu, při syntaktické chybě ukončí překlad s kódem 2
*/
int parse_program(void){
    next();
    // gramatika 
    return 0;
}