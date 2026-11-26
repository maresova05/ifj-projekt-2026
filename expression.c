#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "symtable.h"
#include "scanner.h"
#include "expression.h"


#include <inttypes.h>

#define STACK_INIT_CAPACITY 16
#define STACK_MAX_SIZE 1000
/*
* Struktury, precedenční tabulka
*/
typedef enum{
    PLUS_MINUS_UN, //Unární +-
    NOT,        //Unární !
    MUL_DIV,    //*/
    PLUS_MINUS_BIN, //Binární +-
    REL_CMP, // <, <=, >, >=
    REL_EQ,  // ==, !=
    AND,    //&&
    OR,     //||
    L_PAR, // (
    P_PAR, // )
    ID, //Operandy (proměnné, literály)
    DOLLAR, //Konec výrazu
    SIZE //Velikost matice
}Symbol;

typedef enum{
    S, //Shift <
    R, //Reduce >
    E, //Equal =
    X  //Syntaktická chyba
}Action;

/*const Action precedence_table[SIZE][SIZE] = {
    // +-   */ /* +-  REL (   )   ID  $
    {   S,  R,  R,  R,  S,  R,  S,  R}, // +-
    {   S,  R,  R,  R,  S,  R,  S,  R}, // *//*
    {   S,  S,  R,  R,  S,  R,  S,  R}, // +-
    {   S,  S,  S,  X,  S,  R,  S,  R}, // REL
    {   S,  S,  S,  S,  S,  E,  S,  X}, // (
    {   R,  R,  R,  R,  X,  R,  X,  R}, // )
    {   R,  R,  R,  R,  X,  R,  X,  R}, // ID
    {   S,  S,  S,  S,  S,  X,  S,  X}, // $
};*/

//TABULKA S ROZŠÍŘENÍM BOOLTHEN
const Action precedence_table[SIZE][SIZE] = {
    // +-   !   */  +- CMP  EQ,  &&  ||  (   )   ID  $
    {   S,  S,  R,  R,  R,  R,  R,  R,  S,  R,  S,  R}, // +-
    {   S,  S,  R,  R,  R,  R,  R,  R,  S,  R,  S,  R}, // !
    {   S,  S,  R,  R,  R,  R,  R,  R,  S,  R,  S,  R}, // */
    {   S,  S,  S,  R,  R,  R,  R,  R,  S,  R,  S,  R}, // +-
    {   S,  S,  S,  S,  X,  R,  R,  R,  S,  R,  S,  R}, // REL
    {   S,  S,  S,  S,  S,  X,  R,  R,  S,  R,  S,  R}, // EQ
    {   S,  S,  S,  S,  S,  S,  R,  R,  S,  R,  S,  R}, // &&
    {   S,  S,  S,  S,  S,  S,  S,  R,  S,  R,  S,  R}, // ||
    {   S,  S,  S,  S,  S,  S,  S,  S,  S,  E,  S,  X}, // (
    {   X,  X,  R,  R,  R,  R,  R,  R,  X,  R,  X,  R}, // )
    {   X,  X,  R,  R,  R,  R,  R,  R,  X,  R,  X,  R}, // ID
    {   S,  S,  S,  S,  S,  S,  S,  S,  S,  X,  S,  X}, // $
};

/*
* Fuknce pro zásobník
*/
typedef enum{
    //Terminály
    STACK_PLUS_MINUS_UN = PLUS_MINUS_UN, 
    STACK_MUL_DIV = MUL_DIV,   
    STACK_PLUS_MINUS_BIN = PLUS_MINUS_BIN, 
    STACK_REL_CMP = REL_CMP,
    STACK_REL_EQ = REL_EQ,
    STACK_L_PAR = L_PAR, 
    STACK_R_PAR = P_PAR, 
    STACK_ID = ID, 
    STACK_DOLLAR = DOLLAR, 

    //Speciální znaky
    STACK_STOP,     //zarážka '<'
    STACK_NON_TERM,  //neterminál 'E'   

    //rozšíření
    STACK_NOT = NOT,
    STACK_AND = AND,
    STACK_OR = OR
}StackSymbol;

typedef struct{
    StackSymbol type;
    Token token;
    DataType expr_type;
    int label_id;
} StackItem;

typedef struct {
    StackItem *items;
    int top;
    int capacity;
}Stack;

/**
 * @brief Inicializuje zásobník.
 * 
 * @param s Ukazatel na zásobník.
 * @return false při selhání, true při úspěchu.
 */
bool stack_init(Stack *s){
    if(s==NULL){
        return false;
    }

    s->items = (StackItem *)malloc(STACK_INIT_CAPACITY * sizeof(StackItem));

    if(s->items == NULL){
        return false;
    }

    s->capacity = STACK_INIT_CAPACITY;
    s->top = -1;

    return true;
}

/**
 * @brief Vkládá položku na zásobník.
 * 
 * @param s Ukazatel na inicializovaný zásobník
 * @param item Položka kterou chceme vložit
 * @return false při selhání, true při úspěchu.
 */
bool stack_push(Stack *s, StackItem item){
    if(s ==NULL||s->items==NULL){
        return false;
    }

    if(s->top +1 >=s->capacity){
        int new_capacity = s->capacity*2;
        StackItem *new_items = (StackItem *)realloc(s->items, new_capacity * sizeof(StackItem));
        
        if(new_items == NULL){
            return false;
        }

        s->items = new_items;
        s->capacity = new_capacity;
    }

    s->top++;
    s->items[s->top] = item;

    return true;
}

/**
 * @brief Odebere vrchní položku
 * 
 * @param s Ukazatel na inicializovaný zásobník
 */
void stack_pop(Stack *s){
    if(s->top >= 0){
        s->top --;
    }
}

/**
 * @brief Najde nejvrchnější položku na zásobníku.
 * 
 * @param s Ukazatel na inicializovaný zásobník
 * @return StackItem Nalezenou položku, pokud je prázdný vrací STACK_DOLLAR
 */
StackItem stack_top(Stack *s){
    if(s->top>=0){
        StackItem item = s->items[s->top];
        return item;
    }else{
        StackItem empty_item;
        empty_item.type = STACK_DOLLAR;
        return empty_item;
    }
}

/**
 * @brief Najde nejvrchnější terminál na zásobníku, ignoruje neterminály.
 * 
 * @param s Ukazatel na inicializovaný zásobník
 * @return StackItem První nalezený terminál, pokud je prázdný vrací STACK_DOLLAR
 */
StackItem stack_top_terminal(Stack *s){
    if(s->top>=0){
        for(int i=s->top; i>= 0; i--){
            if((s->items[i].type != STACK_STOP) &&  (s->items[i].type != STACK_NON_TERM)){
                return s->items[i];
            }
        }
    }

    StackItem empty_item;
    empty_item.type = STACK_DOLLAR;
    return empty_item;
}

static void free_token_attr(Token *tok){
    if (tok != NULL && (tok->type == TOKEN_IDENTIFIER || tok->type == TOKEN_STRING_LITERAL)){
        free(tok->attribute.s_val);
        tok->attribute.s_val = NULL;
    }
}

static void free_handle(StackItem *handle, int len){
    for (int i = 0; i < len; i++){
        if (handle[i].type == STACK_ID){
            free_token_attr(&handle[i].token);
        }
    }
}

/**
 * @brief Uvolní zásobník.
 * 
 * @param s Ukazatel na inicializovaný zásobník.
 */
void stack_free(Stack *s){
    if(s == NULL){
        return;
    }
    
    if(s->items != NULL){
        for (int i = 0; i <= s->top; i++){
            if (s->items[i].type == STACK_ID){
                free_token_attr(&s->items[i].token);
            }
        }

        free(s->items);
        s->items = NULL;
    }
    
    s->top = -1;
    s->capacity = 0;
}

/**
 * @brief Mapa převodu tokenu na symbol.
 * 
 * @param t_tyoe Typ tokenu který převádíme.
 */
Symbol map_token_to_symbol(TokenType t_type, int paren_count, bool expect_operand){
    switch(t_type){
        case TOKEN_EOF:
        case TOKEN_EOL:
        case TOKEN_THEN:
        case TOKEN_DO:
        case TOKEN_COMMA:
        case TOKEN_SEMICOLON:
            return DOLLAR;
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return expect_operand ? PLUS_MINUS_UN : PLUS_MINUS_BIN;
        case TOKEN_MUL:
        case TOKEN_DIV:
            return MUL_DIV;
        
        case TOKEN_LESS:
        case TOKEN_LESS_EQ:
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQ:
            return REL_CMP;

        case TOKEN_EQ:
        case TOKEN_NEQ:
            return REL_EQ;

        case TOKEN_LPAREN:
            return L_PAR;
        case TOKEN_RPAREN:
            if(paren_count > 0){
                return P_PAR;
            }else {
                return DOLLAR;
            }
           
        
        case TOKEN_IDENTIFIER:
        case TOKEN_INT_LITERAL:
        case TOKEN_DOUBLE_LITERAL:
        case TOKEN_STRING_LITERAL:
            return ID;
        
        //rozšíření BOOLTHEN
        case TOKEN_NOT:
            return NOT;
        case TOKEN_AND:
            return AND;
        case TOKEN_OR:
            return OR;
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            return ID;
        default:
            return DOLLAR;
    }
}

/**
 * @brief Vygeneruje instrukci PUSHS pro string s převodem znaků ASCII.
 * 
 * @param str Ukazatel na řetězec zakončený znakem '\0'.
 */
static void gen_pushs_string(const char *str){
    printf("PUSHS string@");
    if(str != NULL){
        for(size_t i = 0; str[i] != '\0'; i++){
            unsigned char c = (unsigned char)str[i];
            if(c <= 32 || c == 35 || c == 92 || c>= 127){
                printf("\\%03u", c);
            }else{
                putchar(c);
            }
        }
    }
    printf("\n");
}

/**
 * @brief Funkce pro generování instrukce PUSHS
 * 
 * @param token Token označující typ.
 * @param scope_id TBD//////////////////////////////////
 */
void gen_push_operand(Token token, int scope_id){
    switch (token.type){
        case TOKEN_INT_LITERAL:
            printf("PUSHS int@%lld\n", token.attribute.i_val);
            break;
        
        case TOKEN_DOUBLE_LITERAL:
            printf("PUSHS float@%a\n", token.attribute.d_val);
            break;
        
        case TOKEN_STRING_LITERAL:
            gen_pushs_string(token.attribute.s_val);;
            break;
        
        case TOKEN_IDENTIFIER:
            printf("PUSHS LF@%s$%d\n", token.attribute.s_val, scope_id);
            break;
        
        case TOKEN_TRUE:
            printf("PUSHS bool@true\n");
            break;

        case TOKEN_FALSE:
            printf("PUSHS bool@false\n");
            break;
        default:
            break;
    }
}

/**
 * @brief Funkce pro generování aritmetických operací.
 * 
 * @param token Token označující typ operace.
 */
void gen_operator(TokenType op_type){
    switch(op_type){
        case TOKEN_PLUS:
            printf("ADDS\n");
            break;
        
        case TOKEN_MINUS:
            printf("SUBS\n");
            break;

        case TOKEN_MUL:
            printf("MULS\n");
            break;

        case TOKEN_DIV:
            printf("DIVS\n");
            break;

        case TOKEN_EQ:
            printf("EQS\n");
            break;
        
        case TOKEN_NEQ:
            printf("EQS\n");
            printf("NOTS\n");
            break;

        case TOKEN_LESS:
            printf("LTS\n");
            break;

        case TOKEN_GREATER:
            printf("GTS\n");
            break;
        
        case TOKEN_LESS_EQ:
            printf("POPS GF@$expr_op2\n");
            printf("POPS GF@$expr_op1\n");
            printf("PUSHS GF@$expr_op1\n");
            printf("PUSHS GF@$expr_op2\n");
            printf("LTS\n");
            printf("PUSHS GF@$expr_op1\n");
            printf("PUSHS GF@$expr_op2\n");
            printf("EQS\n");
            printf("ORS\n");
            break;

        case TOKEN_GREATER_EQ:
            printf("POPS GF@$expr_op2\n");
            printf("POPS GF@$expr_op1\n");
            printf("PUSHS GF@$expr_op1\n");
            printf("PUSHS GF@$expr_op2\n");
            printf("GTS\n");
            printf("PUSHS GF@$expr_op1\n");
            printf("PUSHS GF@$expr_op2\n");
            printf("PUSHS GF@$expr_op1\n");
            printf("EQS\n");
            printf("ORS\n");
            break;
        
        default:
            break;
    }
}

static int logic_label_counter = 0;
static ExprResult parse_expression_internal(const SymTableStack *local_stack, DataType *final_type, Token*next_token, bool has_first_token, Token first_token);

static ExprResult parse_function_call(const SymTableStack *local_stack, const char *func_name, DataType *ret_type){
    SymbolData *func = symtable_stack_search(local_stack, func_name);
    if (func == NULL || func->symbol_type != SYMBOL_FUNC){
        return EXPR_ERR_UNDEF;
    }

    size_t arg_count = 0;
    Token tok = {0};
    int lex_rc = get_next_token(&tok);
    if(lex_rc != 0){
        return (lex_rc == 1) ? EXPR_ERR_LEX : EXPR_ERR_INTERNAL;
    }

    if(tok.type != TOKEN_RPAREN){
        bool has_first = true;
        Token first_arg_tok = tok;

        while(1){
            DataType arg_type = TYPE_UNDEFINED;
            Token stop_tok = {0};

            ExprResult rc = parse_expression_internal(local_stack, &arg_type, &stop_tok, has_first, first_arg_tok);

            if(rc != EXPR_OK){
                return rc;
            }
            has_first = false;

            if(arg_count >= func->param_count){
                free_token_attr(&stop_tok);
                return EXPR_ERR_PARAM;
            }

            DataType expect_type = func->param_types[arg_count];
            if(expect_type != TYPE_ANY && arg_type != expect_type){
                free_token_attr(&stop_tok);
                return EXPR_ERR_PARAM;
            }

            arg_count ++;

            if(stop_tok.type == TOKEN_COMMA){
                continue;
            }else if (stop_tok.type == TOKEN_RPAREN){
                break;
            }else{
                free_token_attr(&stop_tok);
                return EXPR_ERR_SYNTAX;
            }
        }
    }

    if(arg_count != func->param_count){
        return EXPR_ERR_PARAM;
    }

    printf("CREATEFRAME\n");
    for (size_t i = arg_count; i> 0; i--){
        printf("DEFVAR TF@%%param%zu\n", i-1);
        printf("POPS TF@%%param%zu\n", i-1);
    }

    printf("CALL $%s\n", func_name);
    printf("PUSHS TF@%%retval\n");

    *ret_type = func->data_type;
    return EXPR_OK;
}
/**
 * Hlavní funkce pro syntaktickou analýzu výrazu.
 * @param local_stack Ukazatel na lokální tabulku symbolů
 * @param final_type Výstupní parametr, zde se uloží výsledný datový typ výrazu
 * @param next_token Výstupní parametr, zde se uloží token na kterém se parser zastavil
 * @return ExprResult Vrací EXPR_OK(0) při úspěchu, nebo příslušný chybový kód.
 */
static ExprResult parse_expression_internal(const SymTableStack *local_stack, DataType *final_type, Token*next_token, bool has_first_token, Token first_token){
    Stack s;
   
    if(!stack_init(&s)){
        return EXPR_ERR_INTERNAL;
    }

    int paren_count = 0;
    bool expect_operand = true;
    Token empty_token = {0};

    StackItem init_item = {STACK_DOLLAR, empty_token, TYPE_UNDEFINED, 0};
    
    if(!stack_push(&s, init_item)){
        stack_free(&s);
        return EXPR_ERR_INTERNAL;
    }

    int lex_rc;
    Token current_token = {0};
    if(has_first_token){
        current_token = first_token;
    }else{
        lex_rc = get_next_token(&current_token);
        if(lex_rc != 0){
            stack_free(&s);
            return (lex_rc == 1) ? EXPR_ERR_LEX : EXPR_ERR_INTERNAL;
        }
    }
    
    if(current_token.type == TOKEN_LPAREN){
        paren_count++;
    }

    Symbol input_symbol = map_token_to_symbol(current_token.type, paren_count, expect_operand);


    while(1){
        StackItem top_term_item = stack_top_terminal(&s);
        Symbol top_term = (Symbol)top_term_item.type;

        Action action = precedence_table[top_term][input_symbol];

        switch(action){
            case S:{
                StackItem top_item = stack_top(&s);
                bool has_non_term = false;
                StackItem non_term_item = {0};

                if(top_item.type == STACK_NON_TERM){
                    has_non_term = true;
                    non_term_item = top_item;
                    stack_pop(&s);
                }

                StackItem stop_item = {STACK_STOP, empty_token, TYPE_UNDEFINED, 0};
                if(!stack_push(&s, stop_item)){
                    free_token_attr(&current_token);
                    stack_free(&s);
                    return EXPR_ERR_INTERNAL;
                }

                if(has_non_term){
                    if(!stack_push(&s, non_term_item)){
                        free_token_attr(&current_token);
                        stack_free(&s);
                        return EXPR_ERR_INTERNAL;
                    }
                }

                int current_label_id = 0;
                if(input_symbol == AND){
                    current_label_id = ++logic_label_counter;
                    printf("POPS GF@$expr_op1\n");
                    printf("PUSHS GF@$expr_op1\n");
                    printf("JUMPIFEQ $and_end_%d GF@$expr_op1 bool@false\n", current_label_id);
                    printf("POPS GF@$expr_op1\n");
                }else if(input_symbol == OR){
                    current_label_id = ++logic_label_counter;
                    printf("POPS GF@$expr_op1\n");
                    printf("PUSHS GF@$expr_op1\n");
                    printf("JUMPIFEQ $or_end_%d GF@$expr_op1 bool@true\n", current_label_id);
                    printf("POPS GF@$expr_op1\n");
                }

                StackItem input_item = {(StackSymbol)input_symbol, current_token, TYPE_UNDEFINED, current_label_id};
                if(!stack_push(&s, input_item)){
                    free_token_attr(&current_token);
                    stack_free(&s);
                    return EXPR_ERR_INTERNAL;
                }
                current_token = empty_token;
                expect_operand = (input_symbol != ID && input_symbol != P_PAR);
                
                lex_rc = get_next_token(&current_token);
                if(lex_rc != 0){
                    stack_free(&s);
                    return (lex_rc == 1) ? EXPR_ERR_LEX : EXPR_ERR_INTERNAL;
                }
                
                if(current_token.type == TOKEN_LPAREN){
                    paren_count++;
                }
                input_symbol = map_token_to_symbol(current_token.type, paren_count, expect_operand);
                break;
            }
            case R:{
                int max_rule_len = 3;
                StackItem handle[3];
                int handle_len = 0;
                bool found_stop = false;

                while(s.top >= 0){
                    StackItem item = stack_top(&s);
                    stack_pop(&s);

                    if(item.type == STACK_STOP){
                        found_stop = true;
                        break;
                    }

                    if(handle_len < max_rule_len){
                        handle[handle_len++] = item;
                       
                    }else{
                        if(item.type == STACK_ID){
                            free_token_attr(&item.token);
                        }
                        free_handle(handle, handle_len);
                        free_token_attr(&current_token);
                        stack_free(&s);
                        return EXPR_ERR_SYNTAX;
                    }
                }

                if(!found_stop){
                    free_handle(handle, handle_len);
                    free_token_attr(&current_token);
                    stack_free(&s);
                    return EXPR_ERR_SYNTAX;
                }

                //bool rule_matched = false;

                //pravidlo E-> id
                if(handle_len == 1 && handle[0].type == STACK_ID){
                   
                    Token operand_token = handle[0].token; 
                    DataType current_type = TYPE_UNDEFINED;
                    int scope_id = 0;

                    if(operand_token.type == TOKEN_IDENTIFIER){
                        SymbolData *sym = symtable_stack_search(local_stack, operand_token.attribute.s_val);

                        if (sym == NULL || sym->symbol_type != SYMBOL_VAR){ 
                            free_handle(handle, handle_len);
                            free_token_attr(&current_token);
                            stack_free(&s);
                            return EXPR_ERR_UNDEF; //proměnná nebyl deklarovaná
                        }

                        current_type = sym->data_type;
                        scope_id = sym->scope_id;
                        
                    }
                    else if(operand_token.type == TOKEN_INT_LITERAL) current_type = TYPE_INT;
                    else if(operand_token.type == TOKEN_DOUBLE_LITERAL) current_type = TYPE_DOUBLE;
                    else if(operand_token.type == TOKEN_STRING_LITERAL) current_type = TYPE_STRING;
                    else if(operand_token.type == TOKEN_TRUE) current_type = TYPE_BOOL;
                    else if(operand_token.type == TOKEN_FALSE) current_type = TYPE_BOOL;

                        gen_push_operand(operand_token, scope_id);
                        free_handle(handle, handle_len);
                    
                    

                    StackItem non_term = {STACK_NON_TERM, empty_token, current_type, 0};
                    if(!stack_push(&s, non_term)){
                        free_token_attr(&current_token);
                        stack_free(&s);
                        return EXPR_ERR_INTERNAL;
                    }

                    continue;
                }
                //pravidlo E -> ()
                if(handle_len == 2 && handle[0].type == STACK_R_PAR && handle[1].type == STACK_L_PAR){
                    printf("PUSHS nil@nil\n");
                    StackItem non_term = {STACK_NON_TERM, empty_token, TYPE_UNIT, 0};

                    if(!stack_push(&s, non_term)){
                        free_token_attr(&current_token);
                        stack_free(&s);
                        return EXPR_ERR_INTERNAL;
                    }
                    continue;
                }
                
                
                //pravidlo E-> E + E (-,*, /)
                if(handle_len == 3 && handle[0].type == STACK_NON_TERM &&
                        (handle[1].type == STACK_PLUS_MINUS_BIN || handle[1].type == STACK_MUL_DIV) &&
                        handle[2].type == STACK_NON_TERM){

                    
                    
                    DataType type1 = handle[2].expr_type;
                    DataType type2 = handle[0].expr_type;
                    DataType result_type;

                    TokenType operand = handle[1].token.type;

                    if(operand == TOKEN_PLUS && type1 == TYPE_STRING && type2 == TYPE_STRING){
                         result_type = TYPE_STRING;

                         printf("POPS GF@$expr_op2\n");
                         printf("POPS GF@$expr_op1\n");
                         printf("CONCAT GF@$expr_op1 GF@$expr_op1 GF@$expr_op2\n");
                         printf("PUSHS GF@$expr_op1\n");

                    }else if(type1 == TYPE_INT && type2 == TYPE_INT){
                        result_type = TYPE_INT;
                        if(operand == TOKEN_DIV){
                            printf("IDIVS\n"); //celočíselné dělení
                        }else{
                            gen_operator(operand);
                        }
                    }else if(type1 == TYPE_DOUBLE && type2==TYPE_DOUBLE){
                        result_type = TYPE_DOUBLE;
                        gen_operator(operand);
                    }else if (type1 == TYPE_DOUBLE && type2 == TYPE_INT){
                        result_type = TYPE_DOUBLE;
                        printf("INT2FLOATS\n");
                        gen_operator(operand);
                    }else if (type1 == TYPE_INT && type2 == TYPE_DOUBLE){
                        result_type = TYPE_DOUBLE;
                        printf("POPS GF@$expr_op2\n"); //v GF nadefinovaná pomocná proměnná
                        printf("INT2FLOATS\n");
                        printf("PUSHS GF@$expr_op2\n");
                        gen_operator(operand);
                    }else{
                        free_token_attr(&current_token);
                        stack_free(&s);
                        return EXPR_ERR_SEMANTIC; //zakázané operace typu Int + String
                    }

                    StackItem non_term = {STACK_NON_TERM, empty_token, result_type, 0};
                    if(!stack_push(&s, non_term)){
                        free_token_attr(&current_token);
                        stack_free(&s);
                        return EXPR_ERR_INTERNAL;
                    }
                    continue;

                }

                //pravidlo E -> E > E (== =!, ...)
                if(handle_len == 3 && handle[0].type == STACK_NON_TERM &&
                        (handle[1].type == STACK_REL_CMP || handle[1].type == STACK_REL_EQ) &&
                        handle[2].type == STACK_NON_TERM){

    
                    DataType type1 = handle[2].expr_type;
                    DataType type2 = handle[0].expr_type;
                    TokenType operand = handle[1].token.type;
                
                    if(type1 == TYPE_DOUBLE && type2 == TYPE_INT){
                        printf("INT2FLOATS\n");
                    }else if(type1 == TYPE_INT && type2 == TYPE_DOUBLE){
                        printf("POPS GF@$expr_op2\n");
                        printf("INT2FLOATS\n");
                        printf("PUSHS GF@$expr_op2\n");
                    }else if(type1 == TYPE_INT && type2 == TYPE_INT){
                        //OK
                    }else if(type1 == TYPE_DOUBLE && type2 == TYPE_DOUBLE){
                        //OK
                    }else if(type1 == TYPE_STRING && type2 == TYPE_STRING){
                        //OK
                    }else if(type1 == type2 && (operand == TOKEN_EQ || operand == TOKEN_NEQ)){
                        //OK
                    }else{
                        free_token_attr(&current_token);
                        stack_free(&s);
                        return EXPR_ERR_SEMANTIC; //zakázané operace typu Int + String
                    }

                    gen_operator(operand);

                    StackItem non_term = {STACK_NON_TERM, empty_token, TYPE_BOOL, 0};
                    if(!stack_push(&s, non_term)){
                        free_token_attr(&current_token);
                        stack_free(&s);
                        return EXPR_ERR_INTERNAL;
                    }
                    continue;

                }//pravidlo E -> E
                /*
                else if(handle_len == 3 &&
                        handle[0].type == STACK_NON_TERM &&
                        handle[1].type == STACK_RELATIONAL &&
                        handle[2].type == STACK_NON_TERM){

                    rule_matched = true;

                    DataType type1 = handle[0].expr_type;
                    DataType type2 = handle[2].expr_type;
                    TokenType operand = handle[1].token.type;
                    
                    DataType result_type = TYPE_BOOL;

                    if(type1 == type2){
                        gen_operator(operand);
                    }else if (type1 == TYPE_DOUBLE && type2 == TYPE_INT){
                        printf("INT2FLOATS\n");
                        gen_operator(operand);
                    }else if (type1 == TYPE_INT && type2 == TYPE_DOUBLE){
                        printf("POPS GF@$expr_op2\n"); //v GF nadefinovaná pomocná proměnná
                        printf("INT2FLOATS\n");
                        printf("PUSHS GF@$expr_op2\n");
                        gen_operator(operand);
                    }else{
                        stack_free(&s);
                        return EXPR_ERR_SEMANTIC; //zakázané operace typu Int < String
                    }

                    StackItem non_term = {STACK_NON_TERM, empty_token, result_type, 0};
                    if(!stack_push(&s, non_term)){
                        stack_free(&s);
                        return EXPR_ERR_INTERNAL;
                    }
                    continue;
                }
                */
                //pravidlo E -> (E)
                if(handle_len == 3 &&
                    handle[0].type == STACK_R_PAR &&
                    handle[1].type == STACK_NON_TERM &&
                    handle[2].type == STACK_L_PAR){
                    
                    
                    DataType result_type = handle[1].expr_type;

                    StackItem non_term = {STACK_NON_TERM, empty_token, result_type, 0};
                    if(!stack_push(&s, non_term)){
                        free_token_attr(&current_token);
                        stack_free(&s);
                        return EXPR_ERR_INTERNAL;
                    }

                    //paren_count--;
                    continue;
                
                }//pravidlo E -> -E (unární operátor)
                if(handle_len == 2 &&
                    handle[0].type == STACK_NON_TERM &&
                    handle[1].type == STACK_PLUS_MINUS_UN){
                    
                    DataType current_type = handle[0].expr_type;

                    if(current_type == TYPE_INT){
                        if(handle[1].token.type == TOKEN_MINUS){
                            printf("POPS GF@$expr_op1\n");
                            printf("PUSHS int@0\n");
                            printf("PUSHS GF@$expr_op1\n");
                            printf("SUBS\n");
                        }
                    }else if(current_type == TYPE_DOUBLE){
                        if(handle[1].token.type == TOKEN_MINUS){
                            printf("POPS GF@$expr_op1\n");
                            printf("PUSHS float@0x0p+0\n");
                            printf("PUSHS GF@$expr_op1\n");
                            printf("SUBS\n");
                        }
                    }else{
                        free_token_attr(&current_token);
                        stack_free(&s);
                        return EXPR_ERR_SEMANTIC; //zakázaná operace unární operátor nad String, Boolean, ...
                    }

                    StackItem non_term = {STACK_NON_TERM, empty_token, current_type, 0};
                    if(!stack_push(&s, non_term)){
                        free_token_attr(&current_token);
                        stack_free(&s);
                        return EXPR_ERR_INTERNAL;
                    }
                    continue;
                }//pravidlo E -> !E
                if(handle_len == 2 && handle[0].type == STACK_NON_TERM && handle[1].type == STACK_NOT){
                    if(handle[0].expr_type != TYPE_BOOL){
                        free_token_attr(&current_token);
                        stack_free(&s);
                        return EXPR_ERR_SEMANTIC;
                    }
                    printf("NOTS\n");
                    StackItem non_term = {STACK_NON_TERM, empty_token, TYPE_BOOL, 0};
                    if(!stack_push(&s, non_term)){
                        free_token_attr(&current_token);
                        stack_free(&s);
                        return EXPR_ERR_INTERNAL;
                    }
                    continue;
                }//pravidlo E -> E && E (||)
                if (handle_len == 3 && handle[0].type == STACK_NON_TERM &&
                        (handle[1].type == STACK_AND || handle[1].type == STACK_OR) &&
                        handle[2].type == STACK_NON_TERM){
                    if (handle[2].expr_type != TYPE_BOOL || handle[0].expr_type != TYPE_BOOL){
                        free_token_attr(&current_token);
                        stack_free(&s);
                        return EXPR_ERR_SEMANTIC;
                    }
                    if(handle[1].type == STACK_AND){
                        printf("LABEL $and_end_%d\n", handle[1].label_id);
                    }else{
                        printf("LABEL $or_end_%d\n", handle[1].label_id);
                    }

                    StackItem non_term = {STACK_NON_TERM, empty_token, TYPE_BOOL, 0};
                    if(!stack_push(&s, non_term)){
                        free_token_attr(&current_token);
                        stack_free(&s);
                        return EXPR_ERR_INTERNAL;
                    }
                    continue;
                }
                
                free_handle(handle, handle_len);
                free_token_attr(&current_token);
                stack_free(&s);
                return EXPR_ERR_SYNTAX;
                
                /*
                StackItem non_term = {STACK_NON_TERM, empty_token, 0, 0};
                if(!stack_push(&s, non_term)){
                    stack_free(&s);
                    return EXPR_ERR_INTERNAL;
                }

                break;
                */
            }

            case E:{
                StackItem input_item = {(StackSymbol)input_symbol, current_token, TYPE_UNDEFINED, 0};
                if(!stack_push(&s, input_item)){
                    free_token_attr(&current_token);
                    stack_free(&s);
                    return EXPR_ERR_INTERNAL;
                }
                current_token = empty_token;

                if (input_symbol == P_PAR && paren_count > 0){
                    paren_count--;
                }
                expect_operand = false;

                lex_rc = get_next_token(&current_token);
                if (lex_rc != 0){
                    stack_free(&s);
                    return (lex_rc == 1) ? EXPR_ERR_LEX : EXPR_ERR_INTERNAL;
                }
                if(current_token.type == TOKEN_LPAREN){
                    paren_count++;
                }
                input_symbol = map_token_to_symbol(current_token.type, paren_count, expect_operand);
                break;
            }

            case X:{
                //FUNEXP: volání funkce id(...)
                if(top_term == ID && input_symbol == L_PAR && top_term_item.token.type == TOKEN_IDENTIFIER
                    && s.top >= 1 && s.items[s.top].type == STACK_ID && s.items[s.top - 1].type == STACK_STOP){
                        StackItem id_item = stack_top(&s);
                        stack_pop(&s);
                        stack_pop(&s);

                        if(paren_count > 0){
                            paren_count --;
                        }
                        current_token = empty_token;

                        DataType func_ret_type = TYPE_UNDEFINED;
                        ExprResult rc = parse_function_call(local_stack, id_item.token.attribute.s_val, &func_ret_type);
                        free_token_attr(&id_item.token);

                        if(rc != EXPR_OK){
                            stack_free(&s);
                            return rc;
                        }

                        StackItem non_term = {STACK_NON_TERM, empty_token, func_ret_type, 0};
                        if(!stack_push(&s, non_term)){
                            stack_free(&s);
                            return EXPR_ERR_INTERNAL;
                        }
                        expect_operand = false;
                        lex_rc = get_next_token(&current_token);
                        if(lex_rc != 0){
                            stack_free(&s);
                            return (lex_rc == 1) ? EXPR_ERR_LEX : EXPR_ERR_INTERNAL;
                        }
                        if(current_token.type == TOKEN_LPAREN){
                            paren_count++;
                        }
                        input_symbol = map_token_to_symbol(current_token.type, paren_count, expect_operand);
                        break;
                    }
                if(top_term == DOLLAR && input_symbol == DOLLAR){
                    if(s.top != 1 || s.items[1].type != STACK_NON_TERM){
                        free_token_attr(&current_token);
                        stack_free(&s);
                        return EXPR_ERR_SYNTAX;
                    }

                    if(final_type != NULL){
                        *final_type = s.items[1].expr_type;
                    }

                    if(next_token != NULL){
                        *next_token = current_token;
                    }else{
                        free_token_attr(&current_token);
                    }

                    stack_free(&s);
                    return EXPR_OK;
                }
                
                free_token_attr(&current_token);
                stack_free(&s);
                return EXPR_ERR_SYNTAX;
            }
                
        }
    }
/*
    StackItem result_item = stack_top(&s);
    if(final_type != NULL){
        *final_type = result_item.expr_type;
    }

    if(next_token != NULL){
        *next_token = current_token;
    }

    stack_free(&s);
    return EXPR_OK;
    */
}

ExprResult parse_expression(const SymTableStack *local_stack, DataType *final_type, Token*next_token){
    Token empty = {0};
    return parse_expression_internal(local_stack, final_type, next_token, false, empty);
}