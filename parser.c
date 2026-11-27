#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "parser.h"
#include "scanner.h"
#include "error.h"
#include "expression.h"
#include "gen_control.h"
#include "symtable.h"
#include "gen_base.h"
#include "input.h"

static Token current; // aktuální token

static int block_depth = 0; // zanoření

static DataType cur_ret_type; // pro zapamatování návratové hodnoty funkce

static bool main_found = false; // kontrola @main

static DataType last_body_type; // pro zjištění návratového typu funkce

// Napojení na tabulku symbolů
static SymTableStack symstack;
#define MAX_PARAMS 32
static char *pnames[MAX_PARAMS];
static DataType ptypes[MAX_PARAMS];
static int pcount;

/**
* @brief založí novou proměnnou
* @param name jméno proměnné
* @param is_mutable false, pokud je value a true, pokud je variable
* @param declared deklarovaný datový typ
* @param expr_type datový typ výrazu
*/
static void sem_var_def(const char *name, bool is_mutable, DataType declared, DataType expr_type){
    if(declared == TYPE_UNDEFINED){
        declared = expr_type;
    }
    if(expr_type != declared){
        report_error(ERR_TYPE,current.line);
    }
    SymbolData s = symtable_make_var(declared, is_mutable,false,symstack.top->scope_id);
    SymTableResult ps = symtable_stack_insert(&symstack,name,&s);
    if(ps == SYMTABLE_EXISTS){
        report_error(ERR_REDEFINITION,current.line);
    }
    gen_defvar(name,symstack.top->scope_id);
    gen_emit("POPS LF@%s$%d\n",name,symstack.top->scope_id);
}

/**
* @brief přiřazení do existující proměnné
* @param name jméno proměnné 
* @param expr_type datový typ výrazu
*/
static void sem_assign(const char *name, DataType expr_type){
    SymbolData *s = symtable_stack_search(&symstack,name);
    if(s == NULL || s->symbol_type != SYMBOL_VAR){
        report_error(ERR_UNDEFINED_SYMBOL,current.line);
    }
    if(s->is_mutable == false){
        report_error(ERR_IMMUTABLE,current.line);
    }
    if(expr_type != s->data_type){
        report_error(ERR_TYPE,current.line);
    }
    gen_emit("POPS LF@%s$%d\n",name,s->scope_id);
}

// pro volání funkcí v libovolném pořadí
static void parse_statement(void);
static void parse_simple_list(void);
static void parse_simple_statement(void);
static bool is_simple_statement_start(void);
static bool is_statement_start(void);
static void parse_if(void);
static void parse_while(void);
static DataType parse_expr_value(void);
static void parse_expr_statement(void);

/**
* @brief ohlásí syntaktickou chybu a ukončí překlad
*/
static void syntax_error(const char *msg){
    fprintf(stderr,"%s\n",msg);
    report_error(ERR_SYNTAX, current.line);
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
        case TOKEN_KW_BOOLEAN:
            t = TYPE_BOOL;
            break;
        default:
            syntax_error("expected type");
    }
    next();
    return t;
}

/**
* @brief parsuje příkazy (pravidla 26 až 28)
*/
static void parse_statement(void){
    if(block_depth == 1){
        last_body_type = TYPE_UNIT;
    }
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
* @brief pokud je hodnota ve funkci uloží ji do retval, pokud v if/else zahodí ji
*/
static void parse_expr_statement(void){
    DataType t = parse_expr_value();
    if(block_depth == 1){
        last_body_type = t;
    }
    if(block_depth == 1 && cur_ret_type != TYPE_UNIT){
        gen_expr_to_retval();
    } else {
        gen_expr_discard();
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
    if(cur_ret_type == TYPE_UNDEFINED){
        report_error(ERR_TYPE,current.line);
    }
    expect(TOKEN_RETURN,"expected return");
    if(current.type == TOKEN_EOL || current.type == TOKEN_SEMICOLON){
        if (cur_ret_type != TYPE_UNIT){
            report_error(ERR_OTHER_SEMANTIC,current.line);
        }
        gen_func_end();
        return;
    }
    scanner_unget(current);
    DataType t = parse_expr_value();
    if(cur_ret_type == TYPE_UNIT){
        gen_expr_discard();
        gen_func_end();
        return;
    }
    if(cur_ret_type != t){
        report_error(ERR_TYPE,current.line);
    }
    gen_return_value();
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
        case TOKEN_TRUE:
        case TOKEN_FALSE:
        case TOKEN_NOT:
            scanner_unget(current);
            parse_expr_statement();
            break;
        case TOKEN_IDENTIFIER:{
            char *name = current.attribute.s_val;
            Token id_tok = current;
            next();
            switch(current.type){
                case TOKEN_ASSIGN:
                    parse_assign(name);
                    break;
                case TOKEN_LPAREN:
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
                case TOKEN_AND:
                case TOKEN_OR:
                    scanner_unget(current);
                    scanner_unget(id_tok);
                    parse_expr_statement();
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
        case TOKEN_TRUE:
        case TOKEN_FALSE:
        case TOKEN_NOT:
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
        case TOKEN_TRUE:
        case TOKEN_FALSE:
        case TOKEN_NOT:
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
    block_depth++;
    symtable_stack_push(&symstack);
    expect(TOKEN_EOL,"expected end of line");
    expect(TOKEN_INDENT,"expected indent");
    parse_statements();
    expect(TOKEN_DEDENT,"expected dedent");
    symtable_stack_pop(&symstack);
    block_depth--;
}

/**
* @brief zpracuje výraz precedenční analýzou, do current uloží token, na kterém výraz skončil
* @return datový typ výrazu 
*/
static DataType parse_expr_value(void){
    DataType type = TYPE_UNDEFINED;
    Token tok = {0};
    ExprResult res = parse_expression(&symstack, &type, &tok);
    if(res != EXPR_OK){
        report_error((Error_code)res,current.line);
    }
    current = tok;
    return type;
}

/**
* @brief zpracuje podmínku if/while, výsledek musí být bool
*/
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
    if(current.type == TOKEN_ELSE){
        next();
        gen_if_else(id);
        if(current.type == TOKEN_IF){
            parse_if();
        } else {
            parse_block();
        }
    } else {
        gen_if_else(id);
    }
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
* @brief parsuje parametr
*/
static void parse_param(void){
    if(pcount >= MAX_PARAMS){
        syntax_error("too many parameters");
    }
    pnames[pcount] = current.attribute.s_val;
    expect(TOKEN_IDENTIFIER,"expected identifier");
    expect(TOKEN_COLON,"expected :");
    ptypes[pcount] = parse_type();
    pcount++;
}

/**
* @brief parsuje seznam parametrů
*/
static void parse_params(void){
    pcount = 0;
    if(current.type == TOKEN_RPAREN){
        return;
    }
    parse_param();
    while(current.type == TOKEN_COMMA){
        expect(TOKEN_COMMA,"expected ,");
        parse_param();
    }
    }

/**
* @brief parsuje anotaci @main 
* @return vrací true nebo false
*/
static bool parse_main_annot(void){
    bool is_main = false;
    if (current.type == TOKEN_MAIN){
        is_main = true;
        next();
        if(current.type == TOKEN_EOL){
            next();
        }
    }
    return is_main;
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
* @brief přeskočí tělo funkce, použití při prvním průchodu
*/
static void skip_body(void){
    expect(TOKEN_ASSIGN,"expected =");
    if(current.type == TOKEN_EOL){
        expect(TOKEN_EOL,"expected end of line");
        expect(TOKEN_INDENT,"expected indent");
        int depth = 1;
        while(depth > 0){
            if(current.type == TOKEN_INDENT){
                depth++;
            }
            if(current.type == TOKEN_DEDENT){
                depth--;
            }
            if(current.type == TOKEN_EOF){
                syntax_error("unexpected end of file");
            }
            next();
        }
    } else {
        while(current.type != TOKEN_EOL){
            if(current.type == TOKEN_EOF){
                syntax_error("unexpected end of file");
            }
            next();
        }
        expect(TOKEN_EOL,"expected end of line");
    }
}

/**
* @brief parsuje definici funkce
* @param name jméno funkce
* @param is_main true, pokud má funkce anotaci @main
* @return návratový typ funkce
*/
static DataType parse_func_header(char **name,bool *is_main){
    *is_main = parse_main_annot();
    expect(TOKEN_DEF,"expected def");
    *name = current.attribute.s_val;
    expect(TOKEN_IDENTIFIER,"expected identifier");
    expect(TOKEN_LPAREN,"expected (");
    parse_params();
    expect(TOKEN_RPAREN,"expected )");
    DataType t = TYPE_UNDEFINED;
    if(current.type == TOKEN_COLON){
        next();
        t = parse_type();
    }
    return t;
}

/**
* @brief zkontroluje @maina vloží funkci s typy parametrů do tabulky symbolů,
* volá se při prvním průchodu, aby šlo funkci volat i před její definicí
* @param name jméno funkce
* @param t návratový typ funkce
* @param is_main true, pokud má funkce anotaci @main
*/
static void declare_function(char *name, DataType t, bool is_main){
    if(is_main){
        if(main_found){
            report_error(ERR_REDEFINITION,current.line);
        }
        if(pcount > 0){
            report_error(ERR_ARGUMENTS,current.line);
        }
        if(t != TYPE_UNIT){
            report_error(ERR_TYPE,current.line);
        }
    main_found = true;
    }
    SymbolData f = symtable_make_func(t,true,true,is_main);
    for(int i = 0;i < pcount;i++){
        symtable_func_add_param(&f,ptypes[i]);
    }
    SymTableResult s = symtable_stack_insert(&symstack,name,&f);
    if(s == SYMTABLE_EXISTS){
        report_error(ERR_REDEFINITION, current.line);
    }
    free(f.param_types);
}

/**
* @brief parsuje definici funkce včetně těla
*/
static void parse_function(void){ // AI: vytvoření kostry funkce
    char *name;
    bool is_main;
    DataType t = parse_func_header(&name,&is_main);
    cur_ret_type = t;
    symtable_stack_push(&symstack);
    for(int i = 0;i < pcount;i++){
        SymbolData v = symtable_make_var(ptypes[i],false,true,0);
        SymTableResult ps = symtable_stack_insert(&symstack,pnames[i],&v);
        if(ps == SYMTABLE_EXISTS){
            report_error(ERR_REDEFINITION, current.line);
        }
    }
    expect(TOKEN_ASSIGN,"expected =");
    if(is_main){
        gen_main_label();
    }
    gen_func_start(name);
    for(int i = 0;i < pcount;i++){
        gen_func_param(pnames[i],symstack.top->scope_id,i);
    }
    if(current.type == TOKEN_EOL){
        parse_block();
    } else {
        scanner_unget(current);
        DataType pt = parse_expr_value();
        if(cur_ret_type == TYPE_UNIT){
            gen_expr_discard();
        } else {
        if (cur_ret_type != TYPE_UNDEFINED && cur_ret_type != pt){
            report_error(ERR_TYPE,current.line);
        }
        last_body_type = pt;
        gen_expr_to_retval();
    }
        expect(TOKEN_EOL,"expected end of line");
    }     
    gen_func_end();
    if(cur_ret_type == TYPE_UNDEFINED){
        SymbolData *fs  = symtable_stack_search(&symstack,name);
        fs->data_type = last_body_type;
    }
    symtable_stack_pop(&symstack);
}

/**
* @brief syntaktická analýza programu rekurzivním sestupem
* @return vrací 0 při úspěchu, při syntaktické chybě ukončí překlad s kódem 2
*/
int parse_program(void){
    symtable_stack_init(&symstack);
    if(symtable_stack_push(&symstack) != SYMTABLE_OK){
        report_error(ERR_INTERNAL, 0);
    }
    if(symtable_init_builtins(&symstack.top->table) != SYMTABLE_OK){
        report_error(ERR_INTERNAL,0);
    }
    // první průchod
    next();
    parse_prolog();
    while(current.type == TOKEN_DEF || current.type == TOKEN_MAIN){
        char *name;
        bool is_main;
        DataType t = parse_func_header(&name,&is_main);
        declare_function(name,t,is_main);
        skip_body();
    }
    expect(TOKEN_EOF,"expected end of file");
    if(!main_found){
        report_error(ERR_UNDEFINED_SYMBOL,current.line);
    }

    input_rewind();
    scanner_reset();

    // druhý průchod
    next();
    parse_prolog();
    while(current.type == TOKEN_DEF || current.type == TOKEN_MAIN){
        parse_function();
    }
    expect(TOKEN_EOF,"expected end of file");
    return 0;
}