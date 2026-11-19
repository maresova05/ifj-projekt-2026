#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "parser.h"
#include "scanner.h"

static Token current; // aktuální token

static void parse_statement(void); // pro volání funkcí v libovolném pořadí
static void parse_simple_list(void);
static void parse_simple_statement(void);
static bool is_simple_statement_start(void);
static bool is_statement_start(void);
static void parse_if(void);
static void parse_while(void);
static void skip_to_end(void);
static void parse_rhs(void);


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
    switch(current.type){
        case TOKEN_KW_INT:
            expect(TOKEN_KW_INT,"expected int");
            break;
        case TOKEN_KW_DOUBLE:
            expect(TOKEN_KW_DOUBLE,"expected double");
            break;
        case TOKEN_KW_STRING:
            expect(TOKEN_KW_STRING,"expected string");
            break;
        case TOKEN_KW_UNIT:
            expect(TOKEN_KW_UNIT,"expected unit");
            break;
        default:
            syntax_error("expected type");
            break;
    }
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
* @brief parsuje příkazy (pravidla 26 až 28)
*/
static void parse_statement(void){
    switch(current.type){
        case TOKEN_IF:
            parse_if();
            break;
        case TOKEN_WHILE:
            parse_while();
            break;
        default:
            if(is_statement_start()){
                parse_simple_list();
                expect(TOKEN_EOL,"expected end of line");
            } else {
                syntax_error("expected statement");
            }
            break;
    }
}

static void parse_var_def(void){
    next();
    expect(TOKEN_IDENTIFIER,"expected identifier");
    if(current.type == TOKEN_COLON){
        next();
        parse_type();
    }
    expect(TOKEN_ASSIGN,"expected =");
    parse_rhs();
}

static void parse_return(void){
    expect(TOKEN_RETURN,"expected return");
    if(current.type == TOKEN_EOL || current.type == TOKEN_SEMICOLON){
        return;
    }
    parse_rhs();
}
static void parse_assign(void){
    expect(TOKEN_ASSIGN,"expected =");
    parse_rhs();
}

static void parse_call(void){
    expect(TOKEN_LPAREN,"expected (");
    if(current.type == TOKEN_IDENTIFIER ||
          current.type == TOKEN_INT_LITERAL ||
          current.type == TOKEN_DOUBLE_LITERAL ||
          current.type == TOKEN_STRING_LITERAL){
            parse_term();
            while(current.type == TOKEN_COMMA){
                next();
                parse_term();
            }
          }
    expect(TOKEN_RPAREN,"expected )");
}

static void skip_to_end(void){
    while(current.type != TOKEN_EOL &&
          current.type != TOKEN_EOF &&
          current.type != TOKEN_SEMICOLON){
            next(); // TODO
          }
}

/**
* @return true, pokud je aktuální token binární operátor
*/
static bool is_binary_op(void){
    switch(current.type){
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_MUL:    
        case TOKEN_DIV:    
        case TOKEN_LESS:    
        case TOKEN_LESS_EQ:    
        case TOKEN_GREATER:    
        case TOKEN_GREATER_EQ:    
        case TOKEN_EQ:    
        case TOKEN_NEQ:    
            return true;
        default:
            return false;
    }
}

/**
* @brief rozhoduje podle prvního a pro identifikátor druhého tokenu (pravidla 34 - 41)
*/
static void parse_simple_statement(void){
    switch(current.type){
        case TOKEN_VAL:
        case TOKEN_VAR:
            parse_var_def();
            break;
        case TOKEN_RETURN:
            parse_return();
            break;
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_INT_LITERAL:
        case TOKEN_DOUBLE_LITERAL:
        case TOKEN_STRING_LITERAL:
        case TOKEN_LPAREN:
            skip_to_end();
            break;
        case TOKEN_IDENTIFIER:
            next();
            switch(current.type){
                case TOKEN_ASSIGN:
                    parse_assign();
                    break;
                case TOKEN_LPAREN:
                    parse_call();
                    break;
                case TOKEN_PLUS:
                case TOKEN_MINUS:
                case TOKEN_MUL:
                case TOKEN_DIV:
                case TOKEN_LESS:
                case TOKEN_LESS_EQ:
                case TOKEN_GREATER:
                case TOKEN_GREATER_EQ:
                case TOKEN_EQ:
                case TOKEN_NEQ:
                case TOKEN_SEMICOLON:
                case TOKEN_EOL:
                    skip_to_end();
                    break;
                default:
                    syntax_error("expected something");
            }
            break;
        default:
            syntax_error("expected something");
    }
}

/**
* @brief parsuje příkazy, dokud v tokenech přichází příkaz -> středník -> příkaz atd.
*/
static void parse_simple_list(void){
    parse_simple_statement();
    while(current.type == TOKEN_SEMICOLON){
        next();
        if(is_simple_statement_start()){
            parse_simple_statement();
        }
    }
}

/**
* @brief pomocná funkce pro určení začátku jednoduchého příkazu
* @return vrací true pokud token může příkaz začínat
*/
static bool is_simple_statement_start(void){
    switch(current.type){
        case TOKEN_IDENTIFIER:
        case TOKEN_RETURN:
        case TOKEN_VAL:    
        case TOKEN_VAR:    
        case TOKEN_PLUS:    
        case TOKEN_MINUS:    
        case TOKEN_INT_LITERAL:    
        case TOKEN_DOUBLE_LITERAL:    
        case TOKEN_STRING_LITERAL:    
        case TOKEN_LPAREN:    
            return true;
        default:
            return false;
    }
}

/**
* @brief určování začátku příkazu
* @return true pokud aktuální token může začínat příkaz, jinak false
*/
static bool is_statement_start(void){
    switch(current.type){
        case TOKEN_WHILE:
        case TOKEN_IDENTIFIER:
        case TOKEN_IF:
        case TOKEN_RETURN:
        case TOKEN_VAL:    
        case TOKEN_VAR:    
        case TOKEN_PLUS:    
        case TOKEN_MINUS:    
        case TOKEN_INT_LITERAL:    
        case TOKEN_DOUBLE_LITERAL:    
        case TOKEN_STRING_LITERAL:    
        case TOKEN_LPAREN:    
            return true;
        default:
            return false;
    }
}

/**
* @brief cyklem projede seznam příkazů
*/
static void parse_statements(void){
    parse_statement();
    while(is_statement_start()){
        parse_statement();
    }
}

/**
* @brief parsuje blok, uvnitř cyklus příkazů, (pravidlo 22)
*/
static void parse_block(void){
    expect(TOKEN_EOL,"expected end of line");
    expect(TOKEN_INDENT,"expected indent");
    parse_statements();
    expect(TOKEN_DEDENT,"expected dedent");
}

// dočasné řešení pro překlad, takto to nebude
static void parse_expr(TokenType type){
    while (current.type != type && current.type != TOKEN_EOF){
        next();
    }
}

/**
* @brief parsuje podmíněný příkaz (pravidlo 67)
*/
static void parse_if(void){
    expect(TOKEN_IF,"expected if");
    parse_expr(TOKEN_THEN);
    expect(TOKEN_THEN,"expected then");
    parse_block();
    expect(TOKEN_ELSE,"expected else");
    parse_block();
}

/**
* @brief parsuje cyklus (pravidlo 68)
*/
static void parse_while(void){
    expect(TOKEN_WHILE,"expected while");
    parse_expr(TOKEN_DO);
    expect(TOKEN_DO,"expected do");
    parse_block();
}

static void parse_rhs(void){
    switch(current.type){
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_INT_LITERAL:
        case TOKEN_DOUBLE_LITERAL:
        case TOKEN_STRING_LITERAL:
        case TOKEN_LPAREN:
            skip_to_end();
            break;
        case TOKEN_IDENTIFIER:
            next();
            if (is_binary_op()){
                skip_to_end();
            } else if(current.type == TOKEN_LPAREN){
                parse_call();
            } else if(current.type == TOKEN_SEMICOLON || current.type == TOKEN_EOL){
                // nothing
            } else {
                syntax_error("expected something");
            }
            break;
        default:
            syntax_error("expected something");
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