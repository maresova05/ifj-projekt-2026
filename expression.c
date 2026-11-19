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
    NOT,        //Unární !
    MUL_DIV,    //*/
    PLUS_MINUS_BIN, //Binární +-
    RELATIONAL, // <, <=, >, >=, ==, !=
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
    // +-   !   */  +- REL  &&  ||  (   )   ID  $
    {   S,  S,  R,  R,  R,  R,  R,  S,  R,  S,  R}, // +-
    {   S,  S,  R,  R,  R,  R,  R,  S,  R,  S,  R}, // !
    {   S,  S,  R,  R,  R,  R,  R,  S,  R,  S,  R}, // */
    {   S,  S,  S,  R,  R,  R,  R,  S,  R,  S,  R}, // +-
    {   S,  S,  S,  S,  X,  R,  R,  S,  R,  S,  R}, // REL
    {   S,  S,  S,  S,  S,  R,  R,  S,  R,  S,  R}, // &&
    {   S,  S,  S,  S,  S,  S,  R,  S,  R,  S,  R}, // ||
    {   S,  S,  S,  S,  S,  S,  S,  S,  E,  S,  X}, // (
    {   X,  X,  R,  R,  R,  R,  R,  X,  R,  X,  R}, // )
    {   X,  X,  R,  R,  R,  R,  R,  X,  R,  X,  R}, // ID
    {   S,  S,  S,  S,  S,  S,  S,  S,  X,  S,  X}, // $
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
        case TOKEN_SEMICOLON:
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
            gen_pushs_string(token.attribute.s_val);;
            break;
        
        case TOKEN_IDENTIFIER:
            printf("PUSHS LF@%s\n", token.attribute.s_val);
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

static int logic_label_counter = 0;


/**
 * Hlavní funkce pro syntaktickou analýzu výrazu.
 * @param global_table Ukazatel na globální tabulku symbolů pro kontrolu deklarací a typů.
 * @param final_type Výstupní parametr, zde se uloží výsledný datový typ výrazu
 * @param next_token Výstupní parametr, zde se uloží token na kterém se parser zastavil
 * @return ExprResult Vrací EXPR_OK(0) při úspěchu, nebo příslušný chybový kód.
 */
ExprResult parse_expression(SymTable *global_table, DataType *final_type, Token*next_token){
    Stack s;
   
    if(!stack_init(&s)){
        return EXPR_ERR_INTERNAL;
    }

    int paren_count = 0;
    Token empty_token = {0};

    StackItem init_item = {STACK_DOLLAR, empty_token, 0, 0};
    
    if(!stack_push(&s, init_item)){
        stack_free(&s);
        return EXPR_ERR_INTERNAL;
    }

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

                StackItem stop_item = {STACK_STOP, empty_token, 0, 0};
                if(!stack_push(&s, stop_item)){
                    stack_free(&s);
                    return EXPR_ERR_INTERNAL;
                }

                if(has_non_term){
                    if(!stack_push(&s, non_term_item)){
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

                StackItem input_item = {(StackSymbol)input_symbol, current_token, 0, current_label_id};
                if(!stack_push(&s, input_item)){
                    stack_free(&s);
                    return EXPR_ERR_INTERNAL;
                }

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
                            return EXPR_ERR_UNDEF; //proměnná nebyl deklarovaná
                        }

                        current_type = node->data_type;
                        gen_push_operand(operand_token);
                        
                    }else{
                        if(operand_token.type == TOKEN_INT_LITERAL) current_type = TYPE_INT;
                        else if(operand_token.type == TOKEN_DOUBLE_LITERAL) current_type = TYPE_DOUBLE;
                        else if(operand_token.type == TOKEN_STRING_LITERAL) current_type = TYPE_STRING;
                        else if(operand_token.type == TOKEN_TRUE) current_type = TYPE_BOOL;
                        else if(operand_token.type == TOKEN_FALSE) current_type = TYPE_BOOL;
                        gen_push_operand(operand_token);
                    }

                    StackItem non_term = {STACK_NON_TERM, empty_token, current_type, 0};
                    if(!stack_push(&s, non_term)){
                        stack_free(&s);
                        return EXPR_ERR_INTERNAL;
                    }

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
                        printf("POPS GF@$expr_op2\n"); //v GF nadefinovaná pomocná proměnná
                        printf("INT2FLOATS\n");
                        printf("PUSHS GF@$expr_op2\n");
                        gen_operator(operand);
                    }else{
                        stack_free(&s);
                        return EXPR_ERR_SEMANTIC; //zakázané operace typu Int + String
                    }

                    StackItem non_term = {STACK_NON_TERM, empty_token, result_type, 0};
                    if(!stack_push(&s, non_term)){
                        stack_free(&s);
                        return EXPR_ERR_INTERNAL;
                    }
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
                        return EXPR_ERR_SEMANTIC; //zakázané operace typu Int + String
                    }

                    gen_operator(operand);

                    StackItem non_term = {STACK_NON_TERM, empty_token, TYPE_BOOL, 0};
                    if(!stack_push(&s, non_term)){
                        stack_free(&s);
                        return EXPR_ERR_INTERNAL;
                    }
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
                //pravidlo E -> (E)
                else if(handle_len == 3 &&
                        handle[0].type == STACK_R_PAR &&
                        handle[1].type == STACK_NON_TERM &&
                        handle[2].type == STACK_L_PAR){
                    
                    rule_matched = true;
                    
                    DataType result_type = handle[1].expr_type;

                    StackItem non_term = {STACK_NON_TERM, empty_token, result_type, 0};
                    if(!stack_push(&s, non_term)){
                        stack_free(&s);
                        return EXPR_ERR_INTERNAL;
                    }

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
                        return EXPR_ERR_SEMANTIC; //zakázaná operace unární operátor nad String, Boolean, ...
                    }

                    StackItem non_term = {STACK_NON_TERM, empty_token, current_type, 0};
                    if(!stack_push(&s, non_term)){
                        stack_free(&s);
                        return EXPR_ERR_INTERNAL;
                    }
                    continue;
                }//pravidlo E -> !E
                else if(handle_len == 2 && handle[0].type == STACK_NON_TERM && handle[1].type == STACK_NOT){
                    rule_matched = true;
                    if(handle[0].expr_type != TYPE_BOOL){
                        stack_free(&s);
                        return EXPR_ERR_SEMANTIC;
                    }
                    printf("NOTS\n");
                    StackItem non_term = {STACK_NON_TERM, empty_token, TYPE_BOOL, 0};
                    if(!stack_push(&s, non_term)){
                        stack_free(&s);
                        return EXPR_ERR_INTERNAL;
                    }
                    continue;
                }//pravidlo E -> E && E (||)
                else if (handle_len == 3 && handle[0].type == STACK_NON_TERM &&
                        (handle[1].type == STACK_AND || handle[1].type == STACK_OR) &&
                        handle[2].type == STACK_NON_TERM){
                    rule_matched = true;
                    if (handle[2].expr_type != TYPE_BOOL || handle[0].expr_type != TYPE_BOOL){
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
                        stack_free(&s);
                        return EXPR_ERR_INTERNAL;
                    }
                    continue;
                }

                if(!rule_matched){
                    stack_free(&s);
                    return EXPR_ERR_SYNTAX;
                }

                StackItem non_term = {STACK_NON_TERM, empty_token, 0, 0};
                if(!stack_push(&s, non_term)){
                    stack_free(&s);
                    return EXPR_ERR_INTERNAL;
                }

                break;
                
            }

            case E:{
                StackItem input_item = {(StackSymbol)input_symbol, current_token, 0, 0};
                if(!stack_push(&s, input_item)){
                    stack_free(&s);
                    return EXPR_ERR_INTERNAL;
                }

                get_next_token(&current_token);
                if(current_token.type == TOKEN_LPAREN){
                    paren_count++;
                }
                input_symbol = map_token_to_symbol(current_token.type, paren_count);
                break;
            }

            case X:{
                if(top_term == DOLLAR && input_symbol == DOLLAR){
                    if(s.top != 1 || s.items[1].type != STACK_NON_TERM){
                        stack_free(&s);
                        return EXPR_ERR_SYNTAX;
                    }

                    if(final_type != NULL){
                        *final_type = s.items[1].expr_type;
                    }

                    if(next_token != NULL){
                        *next_token = current_token;
                    }

                    stack_free(&s);
                    return EXPR_OK;
                }
                
                stack_free(&s);
                return EXPR_ERR_SYNTAX;
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


#ifdef TEST_EXPR

#include <stdio.h>
#include <stdbool.h>

// Globální ukazatel na aktuálně běžící testovací sekvenci tokenů
static const Token *g_test_tokens = NULL;
static int g_test_idx = 0;

// Mockovaná funkce scanneru
int get_next_token(Token *token) {
    *token = g_test_tokens[g_test_idx++];
    return 0; // vrací 0 (úspěch)
}

// Mockovaná funkce tabulky symbolů pro samostatné testy
BSTNode* symtable_search(SymTable table, const char *key) {
    (void)table;
    (void)key;
    return NULL;
}

// Počítadla výsledků
static int g_tests_total = 0;
static int g_tests_passed = 0;

/**
 * Spustí jeden test, porovná návratový kód (a při úspěchu i typ a ukončovací token)
 * a vypíše výsledek na stderr, aby se nemíchal s generovaným IFJcode26 na stdout.
 */
static void run_test(const char *name,
                     const Token *tokens,
                     ExprResult exp_res,
                     DataType exp_type,
                     TokenType exp_stop) {
    g_tests_total++;
    g_test_tokens = tokens;
    g_test_idx = 0;

    DataType actual_type = (DataType)-1;
    Token actual_stop = {0};

    fprintf(stderr, "\n[TEST %02d] %s\n", g_tests_total, name);
    ExprResult actual_res = parse_expression(NULL, &actual_type, &actual_stop);

    bool ok = (actual_res == exp_res);
    if (ok && exp_res == EXPR_OK) {
        if (actual_type != exp_type || actual_stop.type != exp_stop) {
            ok = false;
        }
    }

    if (ok) {
        g_tests_passed++;
        fprintf(stderr, "  -> PASS (kod=%d", actual_res);
        if (exp_res == EXPR_OK) {
            fprintf(stderr, ", typ=%d, stop_token=%d", actual_type, actual_stop.type);
        }
        fprintf(stderr, ")\n");
    } else {
        fprintf(stderr, "  -> FAIL!\n");
        fprintf(stderr, "     Ocekavano: kod=%d", exp_res);
        if (exp_res == EXPR_OK) {
            fprintf(stderr, ", typ=%d, stop_token=%d", exp_type, exp_stop);
        }
        fprintf(stderr, "\n     Ziskano:   kod=%d", actual_res);
        if (actual_res == EXPR_OK) {
            fprintf(stderr, ", typ=%d, stop_token=%d", actual_type, actual_stop.type);
        }
        fprintf(stderr, "\n");
    }
}

int main(void) {
    // --- 1. ARITMETIKA A PRIORITA ---
    {
        Token t[] = {
            { TOKEN_INT_LITERAL, 1, { .i_val = 2 } },
            { TOKEN_PLUS, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 3 } },
            { TOKEN_MUL, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 4 } },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Aritmetika a priorita: 2 + 3 * 4", t, EXPR_OK, TYPE_INT, TOKEN_EOF);
    }
    {
        Token t[] = {
            { TOKEN_LPAREN, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 5 } },
            { TOKEN_PLUS, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 3 } },
            { TOKEN_RPAREN, 1, {0} },
            { TOKEN_MUL, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 2 } },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Zavorky: (5 + 3) * 2", t, EXPR_OK, TYPE_INT, TOKEN_EOF);
    }
    {
        Token t[] = {
            { TOKEN_INT_LITERAL, 1, { .i_val = 10 } },
            { TOKEN_DIV, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 3 } },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Celociselne deleni: 10 / 3 -> Int", t, EXPR_OK, TYPE_INT, TOKEN_EOF);
    }

    // --- 2. IMPLICITNI KONVERZE INT -> DOUBLE ---
    {
        Token t[] = {
            { TOKEN_INT_LITERAL, 1, { .i_val = 5 } },
            { TOKEN_MUL, 1, {0} },
            { TOKEN_DOUBLE_LITERAL, 1, { .d_val = 2.5 } },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Konverze levy Int -> Double: 5 * 2.5", t, EXPR_OK, TYPE_DOUBLE, TOKEN_EOF);
    }
    {
        Token t[] = {
            { TOKEN_DOUBLE_LITERAL, 1, { .d_val = 2.5 } },
            { TOKEN_PLUS, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 5 } },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Konverze pravy Int -> Double: 2.5 + 5", t, EXPR_OK, TYPE_DOUBLE, TOKEN_EOF);
    }

    // --- 3. RETEZCE: KONKATENACE A ESCAPOVANI ---
    {
        Token t[] = {
            { TOKEN_STRING_LITERAL, 1, { .s_val = "Ahoj " } },
            { TOKEN_PLUS, 1, {0} },
            { TOKEN_STRING_LITERAL, 1, { .s_val = "svete" } },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Konkatenace: \"Ahoj \" + \"svete\"", t, EXPR_OK, TYPE_STRING, TOKEN_EOF);
    }
    {
        Token t[] = {
            { TOKEN_STRING_LITERAL, 1, { .s_val = "retezec s lomitkem \\ a\nnovym#radkem" } },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Escapovani retezce (zkontroluj stdout)", t, EXPR_OK, TYPE_STRING, TOKEN_EOF);
    }

    // --- 4. RELACNI OPERATORY ---
    {
        Token t[] = {
            { TOKEN_INT_LITERAL, 1, { .i_val = 5 } },
            { TOKEN_LESS_EQ, 1, {0} },
            { TOKEN_DOUBLE_LITERAL, 1, { .d_val = 5.5 } },
            { TOKEN_THEN, 1, {0} },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Relace + konverze + then: 5 <= 5.5 then", t, EXPR_OK, TYPE_BOOL, TOKEN_THEN);
    }
    {
        Token t[] = {
            { TOKEN_STRING_LITERAL, 1, { .s_val = "abc" } },
            { TOKEN_LESS, 1, {0} },
            { TOKEN_STRING_LITERAL, 1, { .s_val = "abd" } },
            { TOKEN_DO, 1, {0} },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Relace String + do: \"abc\" < \"abd\" do", t, EXPR_OK, TYPE_BOOL, TOKEN_DO);
    }
    {
        Token t[] = {
            { TOKEN_INT_LITERAL, 1, { .i_val = 2 } },
            { TOKEN_PLUS, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 3 } },
            { TOKEN_MUL, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 4 } },
            { TOKEN_EQ, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 14 } },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Priorita aritmetiky pred relaci: 2 + 3 * 4 == 14", t, EXPR_OK, TYPE_BOOL, TOKEN_EOF);
    }

    // --- 5. UKONCOVACI TOKENY A POCITADLO ZAVOREK ---
    {
        Token t[] = {
            { TOKEN_INT_LITERAL, 1, { .i_val = 5 } },
            { TOKEN_PLUS, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 5 } },
            { TOKEN_COMMA, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 10 } },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Zastaveni na carce: 5 + 5 , 10", t, EXPR_OK, TYPE_INT, TOKEN_COMMA);
    }
    {
        Token t[] = {
            { TOKEN_INT_LITERAL, 1, { .i_val = 1 } },
            { TOKEN_PLUS, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 2 } },
            { TOKEN_SEMICOLON, 1, {0} },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Zastaveni na stredniku: 1 + 2 ;", t, EXPR_OK, TYPE_INT, TOKEN_SEMICOLON);
    }
    {
        Token t[] = {
            { TOKEN_LPAREN, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 2 } },
            { TOKEN_PLUS, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 3 } },
            { TOKEN_RPAREN, 1, {0} },
            { TOKEN_MUL, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 5 } },
            { TOKEN_RPAREN, 1, {0} },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Vnorene zavorky + konec argumentu: (2 + 3) * 5 )", t, EXPR_OK, TYPE_INT, TOKEN_RPAREN);
    }

    // --- 6. SEMANTICKE CHYBY (KOD 6) ---
    {
        Token t[] = {
            { TOKEN_STRING_LITERAL, 1, { .s_val = "cislo: " } },
            { TOKEN_PLUS, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 5 } },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Semanticka chyba: \"cislo: \" + 5", t, EXPR_ERR_SEMANTIC, 0, 0);
    }
    {
        Token t[] = {
            { TOKEN_DOUBLE_LITERAL, 1, { .d_val = 5.5 } },
            { TOKEN_MINUS, 1, {0} },
            { TOKEN_STRING_LITERAL, 1, { .s_val = "text" } },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Semanticka chyba: 5.5 - \"text\"", t, EXPR_ERR_SEMANTIC, 0, 0);
    }
    {
        Token t[] = {
            { TOKEN_STRING_LITERAL, 1, { .s_val = "text" } },
            { TOKEN_EQ, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 5 } },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Semanticka chyba: \"text\" == 5", t, EXPR_ERR_SEMANTIC, 0, 0);
    }

    // --- 7. SYNTAKTICKE CHYBY (KOD 2) ---
    {
        Token t[] = {
            { TOKEN_DOUBLE_LITERAL, 1, { .d_val = 3.14 } },
            { TOKEN_MUL, 1, {0} },
            { TOKEN_DIV, 1, {0} },
            { TOKEN_DOUBLE_LITERAL, 1, { .d_val = 2.0 } },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Syntakticka chyba: 3.14 * / 2.0", t, EXPR_ERR_SYNTAX, 0, 0);
    }
    {
        Token t[] = {
            { TOKEN_INT_LITERAL, 1, { .i_val = 1 } },
            { TOKEN_LESS, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 2 } },
            { TOKEN_LESS, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 3 } },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Syntakticka chyba (retezeni relaci): 1 < 2 < 3", t, EXPR_ERR_SYNTAX, 0, 0);
    }
    {
        Token t[] = {
            { TOKEN_LPAREN, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 5 } },
            { TOKEN_PLUS, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 3 } },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Syntakticka chyba (neuzavrena zavorka): (5 + 3", t, EXPR_ERR_SYNTAX, 0, 0);
    }
    {
        Token t[] = {
            { TOKEN_EOF, 1, {0} }
        };
        run_test("Syntakticka chyba (prazdny vyraz)", t, EXPR_ERR_SYNTAX, 0, 0);
    }
    // --- 8. BOOLTHEN: LITERALY, NEGACE, PRIORITY A ZKRATOVE VYHODNOCOVANI ---
    {
        Token t[] = {
            { TOKEN_TRUE, 1, {0} },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("BOOLTHEN literal: true", t, EXPR_OK, TYPE_BOOL, TOKEN_EOF);
    }
    {
        Token t[] = {
            { TOKEN_NOT, 1, {0} },
            { TOKEN_TRUE, 1, {0} },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("BOOLTHEN negace: !true", t, EXPR_OK, TYPE_BOOL, TOKEN_EOF);
    }
    {
        Token t[] = {
            { TOKEN_NOT, 1, {0} },
            { TOKEN_NOT, 1, {0} },
            { TOKEN_FALSE, 1, {0} },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("BOOLTHEN dvojita negace (prava asoc.): !!false", t, EXPR_OK, TYPE_BOOL, TOKEN_EOF);
    }
    {
        Token t[] = {
            { TOKEN_TRUE, 1, {0} },
            { TOKEN_EQ, 1, {0} },
            { TOKEN_FALSE, 1, {0} },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("BOOLTHEN porovnani: true == false", t, EXPR_OK, TYPE_BOOL, TOKEN_EOF);
    }
    {
        Token t[] = {
            { TOKEN_TRUE, 1, {0} },
            { TOKEN_OR, 1, {0} },
            { TOKEN_FALSE, 1, {0} },
            { TOKEN_AND, 1, {0} },
            { TOKEN_FALSE, 1, {0} },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("BOOLTHEN priorita && nad ||: true || false && false", t, EXPR_OK, TYPE_BOOL, TOKEN_EOF);
    }
    {
        // POZOR: uprav TOKEN_LESS podle nazvu ve vasem scanner.h (napr. TOKEN_LESS)
        Token t[] = {
            { TOKEN_INT_LITERAL, 1, { .i_val = 1 } },
            { TOKEN_LESS, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 2 } },
            { TOKEN_AND, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 3 } },
            { TOKEN_EQ, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 3 } },
            { TOKEN_OR, 1, {0} },
            { TOKEN_FALSE, 1, {0} },
            { TOKEN_THEN, 1, {0} },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("BOOLTHEN relace + && + || + then: 1 < 2 && 3 == 3 || false then", t, EXPR_OK, TYPE_BOOL, TOKEN_THEN);
    }
    {
        Token t[] = {
            { TOKEN_NOT, 1, {0} },
            { TOKEN_LPAREN, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 2 } },
            { TOKEN_PLUS, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 3 } },
            { TOKEN_EQ, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 5 } },
            { TOKEN_RPAREN, 1, {0} },
            { TOKEN_AND, 1, {0} },
            { TOKEN_NOT, 1, {0} },
            { TOKEN_FALSE, 1, {0} },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("BOOLTHEN kombinace: !(2 + 3 == 5) && !false", t, EXPR_OK, TYPE_BOOL, TOKEN_EOF);
    }

    // --- 9. BOOLTHEN: SEMANTICKE CHYBY (KOD 6) ---
    {
        Token t[] = {
            { TOKEN_NOT, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 5 } },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("BOOLTHEN semanticka chyba: !5", t, EXPR_ERR_SEMANTIC, 0, 0);
    }
    {
        Token t[] = {
            { TOKEN_TRUE, 1, {0} },
            { TOKEN_AND, 1, {0} },
            { TOKEN_INT_LITERAL, 1, { .i_val = 1 } },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("BOOLTHEN semanticka chyba: true && 1", t, EXPR_ERR_SEMANTIC, 0, 0);
    }
    {
        Token t[] = {
            { TOKEN_INT_LITERAL, 1, { .i_val = 5 } },
            { TOKEN_OR, 1, {0} },
            { TOKEN_TRUE, 1, {0} },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("BOOLTHEN semanticka chyba: 5 || true", t, EXPR_ERR_SEMANTIC, 0, 0);
    }
    {
        Token t[] = {
            { TOKEN_TRUE, 1, {0} },
            { TOKEN_PLUS, 1, {0} },
            { TOKEN_FALSE, 1, {0} },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("BOOLTHEN semanticka chyba: true + false", t, EXPR_ERR_SEMANTIC, 0, 0);
    }
    {
        // POZOR: uprav TOKEN_LESS podle nazvu ve vasem scanner.h
        // Operatory <, <=, >, >= jsou jen pro cisla nebo retezce, ne pro Boolean
        Token t[] = {
            { TOKEN_TRUE, 1, {0} },
            { TOKEN_LESS, 1, {0} },
            { TOKEN_FALSE, 1, {0} },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("BOOLTHEN semanticka chyba: true < false", t, EXPR_ERR_SEMANTIC, 0, 0);
    }

    // --- 10. BOOLTHEN: SYNTAKTICKE CHYBY (KOD 2) ---
    {
        Token t[] = {
            { TOKEN_TRUE, 1, {0} },
            { TOKEN_AND, 1, {0} },
            { TOKEN_OR, 1, {0} },
            { TOKEN_FALSE, 1, {0} },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("BOOLTHEN syntakticka chyba: true && || false", t, EXPR_ERR_SYNTAX, 0, 0);
    }
    {
        Token t[] = {
            { TOKEN_TRUE, 1, {0} },
            { TOKEN_NOT, 1, {0} },
            { TOKEN_FALSE, 1, {0} },
            { TOKEN_EOF, 1, {0} }
        };
        run_test("BOOLTHEN syntakticka chyba: true ! false", t, EXPR_ERR_SYNTAX, 0, 0);
    }

    // --- SOUHRN ---
    fprintf(stderr, "\n========================================\n");
    fprintf(stderr, "VYSLEDEK: %d / %d testu proslo.\n", g_tests_passed, g_tests_total);
    fprintf(stderr, "========================================\n");

    return (g_tests_passed == g_tests_total) ? 0 : 1;
}

#endif // TEST_EXPR