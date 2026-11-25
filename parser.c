#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "parser.h"
#include "scanner.h"
#include "error.h"
#include "expression.h"
#include "gen_control.h"

static Token current; // aktuální token

// TODO: nahradit funkcemi
static void sem_var_def(const char *name, bool is_mutable, DataType declared, DataType expr_type){
    (void)name; (void)is_mutable; (void)declared; (void)expr_type;
}
static void sem_assign(const char *name, DataType expr_type){
    (void)name; (void)expr_type;
}

// pro volání funkcí v libovolném pořadí
static void parse_statement(void);
static void parse_simple_list(void);
static void parse_simple_statement(void);
static bool is_simple_statement_start(void);
static bool is_statement_start(void);
static void parse_if(void);
static void parse_while(void);
static void skip_to_end(void);
static void parse_rhs(void);
static DataType parse_expr_value(void);

/**
* @brief ohlásí syntaktickou chybu a ukončí překlad
*/
static void syntax_error(const char *msg){
    fprintf(stderr,"Syntax error on line %d: %s\n",current.line,msg);
    report_error(ERR_SYNTAX, 0);
}

/**
* @brief načte od lexeru další token 
*/
static void next(void){
    int rc = get_next_token(&current);
    if(rc == 1){
        report_error(ERR_LEXICAL, current.line);
    } else if(rc == 0){
        // nothing
    } else {
        report_error(ERR_INTERNAL,current.line);
    }
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
static DataType parse_type(void){
    DataType t = TYPE_UNDEFINED;
    switch(current.type){
        case TOKEN_KW_INT:
            t = TYPE_INT;
            break;
        case TOKEN_KW_DOUBLE:
            t = TYPE_DOUBLE;
            break;
        case TOKEN_KW_STRING:
            t = TYPE_STRING;
            break;
        case TOKEN_KW_UNIT:
            t = TYPE_UNIT;
            break;
        default:
            syntax_error("expected type");
    }
    next();
    return t;
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

/**
* @brief parsuje definici proměnné val/var
*/
static void parse_var_def(void){
    bool is_mutable = (current.type == TOKEN_VAR);
    next();
    if(current.type != TOKEN_IDENTIFIER){ syntax_error("expected identifier"); }
    char *name = current.attribute.s_val;
    next();
    DataType declared = TYPE_UNDEFINED;
    if(current.type == TOKEN_COLON){
        next();
        declared = parse_type();
    }
    if(current.type != TOKEN_ASSIGN){
        syntax_error("expected =");
    }
    DataType t = parse_expr_value();
    sem_var_def(name, is_mutable, declared, t);
}

/**
* @brief parsuje return s volitelnou hodnotou
*/
static void parse_return(void){
    expect(TOKEN_RETURN,"expected return");
    if(current.type == TOKEN_EOL || current.type == TOKEN_SEMICOLON){
        return;
    }
    parse_rhs();
}

/**
* @brief parsuje přiřazení 
*/
static void parse_assign(const char *name){
    if(current.type != TOKEN_ASSIGN){
        syntax_error("expected =");
    }
    DataType t = parse_expr_value();
    sem_assign(name,t);
}

/**
* @brief parsuje volání funkce a seznam argumentů
*/
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

/**
* @brief přeskočí výraz do EOL nebo středníku
*/
static void skip_to_end(void){ // TODO vyrazy
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
        case TOKEN_IDENTIFIER:{
            char *name = current.attribute.s_val;
            next();
            switch(current.type){
                case TOKEN_ASSIGN:
                    parse_assign(name);
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
        }
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

static DataType parse_expr_value(void){
    DataType type = TYPE_UNDEFINED;
    Token tok = {0};
    ExprResult res = parse_expression(NULL, &type, &tok); // TODO SymTableStack
    if(res != EXPR_OK){
        report_error((Error_code)res,current.line);
    }
    current = tok;
    return type;
}

static void parse_condition(void){
    DataType type = parse_expr_value();
    if(type != TYPE_BOOL){
        report_error(ERR_TYPE,current.line);
    }
}

/**
* @brief parsuje podmíněný příkaz (pravidlo 67)
*/
static void parse_if(void){
    int id = gen_new_label_id();
    if(current.type != TOKEN_IF){
        syntax_error("expected if");
    }
        parse_condition();
        expect(TOKEN_THEN,"expected then");
        gen_if_start(id);
        parse_block();
        expect(TOKEN_ELSE,"expected else");
        gen_if_else(id);
        parse_block();
        gen_if_end(id);
}

/**
* @brief parsuje cyklus (pravidlo 68)
*/
static void parse_while(void){
    int id = gen_new_label_id();
    if(current.type != TOKEN_WHILE){
        syntax_error("expected while");
    }
    gen_while_start(id);
    parse_condition();
    expect(TOKEN_DO,"expected do");
    gen_while_cond(id);
    parse_block();
    gen_while_end(id);
}

/**
* @brief parsuje pravou stranu přiřazení, výrazy zatím přeskakuje
*/
static void parse_rhs(void){ // TODO vyrazy
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
* @brief parsuje parametr
*/
static void parse_param(void){
    expect(TOKEN_IDENTIFIER,"expected identifier");
    expect(TOKEN_COLON,"expected :");
    parse_type();
}

/**
* @brief parsuje seznam parametrů
*/
static void parse_params(void){
    if(current.type == TOKEN_IDENTIFIER){
        parse_param();
        while(current.type == TOKEN_COMMA){
            next();
            parse_param();
        }
    }
}

/**
* @brief parsuje anotaci @main 
*/
static void parse_main_annot(void){
    if (current.type == TOKEN_MAIN){
        next();
        if(current.type == TOKEN_EOL){
            next();
        }
    }
}

/**
* @brief parsuje úvodní import
*/
static void parse_prolog(void){
    expect(TOKEN_IMPORT,"expected import");
    expect(TOKEN_IDENTIFIER,"expected identifier");
    expect(TOKEN_DOT,"expected .");
    expect(TOKEN_MUL,"expected *");
    expect(TOKEN_EOL,"expected end of line");
}

/**
* @brief parsuje definici funkce včetně těla
*/
static void parse_function(void){
    parse_main_annot();
    expect(TOKEN_DEF,"expected def");
    char *name = current.attribute.s_val;
    expect(TOKEN_IDENTIFIER,"expected identifier");
    expect(TOKEN_LPAREN,"expected (");
    parse_params();
    expect(TOKEN_RPAREN,"expected )");
    expect(TOKEN_COLON,"expected :");
    parse_type();
    expect(TOKEN_ASSIGN,"expected =");
    gen_func_start(name);
    if(current.type == TOKEN_EOL){
        parse_block();
    } else {
        parse_rhs();
        expect(TOKEN_EOL,"expected end of line");
    }
    gen_func_end();
}

/**
* @brief syntaktická analýza programu rekurzivním sestupem
* @return vrací 0 při úspěchu, při syntaktické chybě ukončí překlad s kódem 2
*/
int parse_program(void){
    next();
    parse_prolog();
    while(current.type == TOKEN_DEF || current.type == TOKEN_MAIN){
        parse_function();
    }
    expect(TOKEN_EOF,"expected end of file");
    return 0;
}