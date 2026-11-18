#define __USE_MINGW_ANSI_STDIO 1

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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
    MUL_DIV,    //*/
    PLUS_MINUS_BIN, //Binární +-
    RELATIONAL, // <, <=, >, >=, ==, !=
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

const Action precedence_table[SIZE][SIZE] = {
    // +-   */  +-  REL (   )   ID  $
    {   S,  R,  R,  R,  S,  R,  S,  R}, // +-
    {   S,  R,  R,  R,  S,  R,  S,  R}, // */
    {   S,  S,  R,  R,  S,  R,  S,  R}, // +-
    {   S,  S,  S,  X,  S,  R,  S,  R}, // REL
    {   S,  S,  S,  S,  S,  E,  S,  X}, // (
    {   R,  R,  R,  R,  X,  R,  X,  R}, // )
    {   R,  R,  R,  R,  X,  R,  X,  R}, // ID
    {   S,  S,  S,  S,  S,  X,  S,  X}, // $
};

/*
* Fuknce pro zásobník
*/
typedef enum{
    //Terminály
    STACK_PLUS_MINUS_UN = PLUS_MINUS_UN, 
    STACK_MUL_DIV = MUL_DIV,   
    STACK_PLUS_MINUS_BIN = PLUS_MINUS_BIN, 
    STACK_RELATIONAL = RELATIONAL,
    STACK_L_PAR = L_PAR, 
    STACK_R_PAR = P_PAR, 
    STACK_ID = ID, 
    STACK_DOLLAR = DOLLAR, 

    //Speciální znaky
    STACK_STOP,     //zarážka '<'
    STACK_NON_TERM  //neterminál 'E'   
}StackSymbol;

typedef struct{
    StackSymbol type;
    Token token;
    DataType expr_type;
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
Symbol map_token_to_symbol(TokenType t_type, int paren_count){
    switch(t_type){
        case TOKEN_EOF:
        case TOKEN_EOL:
        case TOKEN_THEN:
        case TOKEN_DO:
        case TOKEN_COMMA:
            return DOLLAR;
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return PLUS_MINUS_BIN;
        case TOKEN_MUL:
        case TOKEN_DIV:
            return MUL_DIV;
        

        case TOKEN_EQ:
        case TOKEN_NEQ:
        case TOKEN_LESS:
        case TOKEN_LESS_EQ:
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQ:
            return RELATIONAL;

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

        default:
            return DOLLAR;
    }
}

/**
 * @brief Funkce pro generování instrukce PUSHS
 * 
 * @param token Token který chceme vložit.
 */
void gen_push_operand(Token token){
    switch (token.type){
        case TOKEN_INT_LITERAL:
            printf("PUSHS int@%" PRId64 "\n", token.attribute.i_val);
            break;
        
        case TOKEN_DOUBLE_LITERAL:
            printf("PUSHS float@%a\n", token.attribute.d_val);
            break;
        
        case TOKEN_STRING_LITERAL:
            printf("PUSHS string@%s\n", token.attribute.s_val);
            break;
        
        case TOKEN_IDENTIFIER:
            printf("PUSHS LF@%s\n", token.attribute.s_val);
            break;
        
        default:
            break;
    }
}


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
            printf("NEQS\n");
            break;

        case TOKEN_LESS:
            printf("LTS\n");
            break;

        case TOKEN_GREATER:
            printf("GTS\n");
            break;
        
        case TOKEN_LESS_EQ:
            printf("GTS\n");
            break;

        case TOKEN_GREATER_EQ:
            printf("LTS\n");
            break;
        
        default:
            break;
    }
}

/**
 * Hlavní funkce pro syntaktickou analýzu výrazu.
 * @param global_table Ukazatel na globální tabulku symbolů pro kontrolu deklarací a typů.
 * @param final_type Výstupní parametr, zde se uloží výsledný datový typ výrazu
 * @param next_token Výstupní parametr, zde se uloží token na kterém se parser zastavil
 * @return ExprResult Vrací EXPR_OK(0) při úspěchu, nebo příslušný chybový kód.
 */
ExprResult parse_expression(SymTable *global_table, DataType *final_type, Token*next_token){
    Stack s;
    stack_init(&s);
    int paren_count = 0;
    Token empty_token = {0};

    StackItem init_item = {STACK_DOLLAR, empty_token, 0};
    stack_push(&s, init_item);

    Token current_token;
    get_next_token(&current_token);

    if(current_token.type == TOKEN_LPAREN){
        paren_count++;
    }

    Symbol input_symbol = map_token_to_symbol(current_token.type, paren_count);

    bool success = false;

    while(!success){
        StackItem top_item = stack_top_terminal(&s);
        Symbol top_term = (Symbol)top_item.type;

        Action action = precedence_table[top_term][input_symbol];

        switch(action){
            case S:{
                StackItem top_item = stack_top(&s);
                bool has_non_term = false;
                StackItem non_term_item;

                if(top_item.type == STACK_NON_TERM){
                    has_non_term = true;
                    non_term_item = top_item;
                    stack_pop(&s);
                }

                StackItem stop_item = {STACK_STOP, empty_token, 0};
                stack_push(&s, stop_item);

                if(has_non_term){
                    stack_push(&s, non_term_item);
                }

                StackItem input_item = {(StackSymbol)input_symbol, current_token, 0};
                stack_push(&s, input_item);

                get_next_token(&current_token);
                if(current_token.type == TOKEN_LPAREN){
                    paren_count++;
                }
                input_symbol = map_token_to_symbol(current_token.type, paren_count);
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
                        handle[handle_len] = item;
                        handle_len++;
                    }else{
                        stack_free(&s);
                        return EXPR_ERR_SYNTAX;
                    }
                }

                if(!found_stop){
                    stack_free(&s);
                    return EXPR_ERR_SYNTAX;
                }

                bool rule_matched = false;

                //pravidlo E-> id
                if(handle_len == 1 && handle[0].type == STACK_ID){
                    rule_matched = true;
                    Token operand_token = handle[0].token; 
                    DataType current_type;

                    if(operand_token.type == TOKEN_IDENTIFIER){
                        BSTNode *node = symtable_search(*global_table, operand_token.attribute.s_val);

                        if (node == NULL){ 
                            stack_free(&s);
                            return EXPR_ERR_SEMANTC; //proměnná nebyl deklarovaná
                        }

                        current_type = node->data_type;
                        gen_push_operand(operand_token);
                        
                    }else{
                        if(operand_token.type == TOKEN_INT_LITERAL) current_type = TYPE_INT;
                        if(operand_token.type == TOKEN_DOUBLE_LITERAL) current_type = TYPE_DOUBLE;
                        if(operand_token.type == TOKEN_STRING_LITERAL) current_type = TYPE_STRING;

                        gen_push_operand(operand_token);
                    }

                    StackItem non_term = {STACK_NON_TERM, empty_token, current_type};
                    stack_push(&s, non_term);

                    continue;
                } //pravidlo E-> E + E (-,*, /, ==, ...)
                else if(handle_len == 3 && handle[0].type == STACK_NON_TERM &&
                        (handle[1].type == STACK_PLUS_MINUS_BIN || handle[1].type == STACK_MUL_DIV) &&
                        handle[2].type == STACK_NON_TERM){

                    rule_matched = true;
                    
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
                        printf("POPS GF@temp_op\n"); //v GF nadefinovaná pomocná proměnná
                        printf("INT2FLOATS\n");
                        printf("PUSHS GF@temp_op\n");
                        gen_operator(operand);
                    }else{
                        stack_free(&s);
                        return EXPR_ERR_SEMANTC; //zakázané operace typu Int + String
                    }

                    StackItem non_term = {STACK_NON_TERM, empty_token, result_type};
                    stack_push(&s, non_term);
                    continue;

                }//pravidlo E -> E > E (== =!)
                else if(handle_len == 3 && handle[0].type == STACK_NON_TERM &&
                        handle[1].type == STACK_RELATIONAL &&
                        handle[2].type == STACK_NON_TERM){

                    rule_matched = true;
                    
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
                        stack_free(&s);
                        return EXPR_ERR_SEMANTC; //zakázané operace typu Int + String
                    }

                    gen_operator(operand);

                    StackItem non_term = {STACK_NON_TERM, empty_token, TYPE_BOOL};
                    stack_push(&s, non_term);
                    continue;

                }//pravidlo E -> E
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
                        printf("POPS GF@temp_op\n"); //v GF nadefinovaná pomocná proměnná
                        printf("INT2FLOATS\n");
                        printf("PUSHS GF@temp_op\n");
                        gen_operator(operand);
                    }else{
                        stack_free(&s);
                        return EXPR_ERR_SEMANTC; //zakázané operace typu Int < String
                    }

                    StackItem non_term = {STACK_NON_TERM, empty_token, result_type};
                    stack_push(&s, non_term);
                    continue;
                }
                //pravidlo E -> (E)
                else if(handle_len == 3 &&
                        handle[0].type == STACK_R_PAR &&
                        handle[1].type == STACK_NON_TERM &&
                        handle[2].type == STACK_L_PAR){
                    
                    rule_matched = true;
                    
                    DataType result_type = handle[1].expr_type;

                    StackItem non_term = {STACK_NON_TERM, empty_token, result_type};
                    stack_push(&s, non_term);

                    paren_count--;
                    continue;
                
                }//pravidlo E -> -E (unární operátor)
                else if(handle_len == 2 &&
                        handle[0].type == STACK_NON_TERM &&
                        handle[1].type == STACK_PLUS_MINUS_UN){
                    
                    rule_matched = true;
                    DataType current_type = handle[0].expr_type;

                    if(current_type == TYPE_INT){
                        if(handle[1].token.type == TOKEN_MINUS){
                            printf("POPS GF@tmp_op\n");
                            printf("PUSHS int@0\n");
                            printf("PUSHS GF@tmp_op\n");
                            printf("SUBS\n");
                        }
                    }else if(current_type == TYPE_DOUBLE){
                        if(handle[1].token.type == TOKEN_MINUS){
                            printf("POPS GF@tmp_op\n");
                            printf("PUSHS float@0.0\n");
                            printf("PUSHS GF@tmp_op\n");
                            printf("SUBS\n");
                        }
                    }else{
                        stack_free(&s);
                        return EXPR_ERR_SEMANTC; //zakázaná operace unární operátor nad String, Boolean, ...
                    }

                    StackItem non_term = {STACK_NON_TERM, empty_token, current_type};
                    stack_push(&s, non_term);
                    continue;
                }

                if(!rule_matched){
                    stack_free(&s);
                    return EXPR_ERR_SYNTAX;
                }

                StackItem non_term = {STACK_NON_TERM, empty_token, 0};
                stack_push(&s, non_term);

                break;
                
            }

            case E:{
                StackItem input_item = {(StackSymbol)input_symbol, current_token, 0};
                stack_push(&s, input_item);

                get_next_token(&current_token);
                if(current_token.type == TOKEN_LPAREN){
                    paren_count++;
                }
                input_symbol = map_token_to_symbol(current_token.type, paren_count);
                break;
            }

            case X:{
                if(top_term == DOLLAR && input_symbol == DOLLAR){
                    success = true;
                }else{
                    stack_free(&s);
                    return EXPR_ERR_SYNTAX;
                }
                break;
            }
                
        }
    }

    StackItem result_item = stack_top(&s);
    if(final_type != NULL){
        *final_type = result_item.expr_type;
    }

    if(next_token != NULL){
        *next_token = current_token;
    }

    stack_free(&s);
    return EXPR_OK;
}


// =========================================================================
// TESTOVACÍ BLOK (Blok 2 - Sémantika)
// =========================================================================

// Dummy tabulka symbolů pro náš test (aby kompilátor nehlásil chybu)
BSTNode* symtable_search(SymTable table, const char* key) {
    (void)table;
    (void)key;
    return NULL; 
}

SymTable global_dummy_table = NULL;

// Testovací výraz
Token test_sequence[] = {
    { TOKEN_INT_LITERAL, 1, { .i_val = 1 } },
    { TOKEN_LESS, 1, {0} },
    { TOKEN_INT_LITERAL, 1, { .i_val = 2 } },
    { TOKEN_LESS, 1, {0} },
    { TOKEN_INT_LITERAL, 1, { .i_val = 3 } },
    { TOKEN_EOF, 1, {0} }
};
int token_index = 0;

int get_next_token(Token *token) {
    *token = test_sequence[token_index];
    if (token->type != TOKEN_EOF && token->type != TOKEN_EOL) {
        token_index++;
    }
    return 1;
}


int main() {
    printf("--- START SÉMANTICKÉHO PARSERU ---\n");
    
   
    DataType final_type;
    Token next_token;

    
    ExprResult result = parse_expression(&global_dummy_table, &final_type, &next_token);
    
    printf("--- KONEC PARSERU ---\n");
   if (result == EXPR_OK) {
        printf("Preklad uspesny! (Kod: %d)\n", result);
        printf("-> Vysledny datovy typ: %d\n", final_type);
        printf("-> Vyraz zastaven na tokenu: %d\n", next_token.type);
    } else {
        printf("Preklad selhal s chybovym kodem: %d\n", result);
    }
    
    return 0;
}