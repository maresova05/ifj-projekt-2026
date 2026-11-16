#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "scanner.h"

static Token current; // aktuální token

// ohlásí syntaktickou chybu a ukončí překlad
static void syntax_error(const char *msg){
    fprintf(stderr,"Syntax error on line %d: %s\n",current.line,msg);
    exit(2);
}

// načte od lexeru další token
static void next(void){
    get_next_token(&current);
}

// ověří očekávaný typ tokenu a přesune se na další
static void expect(TokenType type, const char *msg){
    if (current.type != type){
        syntax_error(msg);
    }
    next();
}

int parse_program(void){
    next();
    // gramatika 
    return 0;
}