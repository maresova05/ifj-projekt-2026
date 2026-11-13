#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "expression.h"

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
 * @brief Hlavní funkce pro syntaktickou analýzu výrazu
 * 
 * @return ExprResult Vrací EXPR_OK při úspěchu, nebo příslušný chybový kód 
 *         při selhání
 */
ExprResult parse_expression(){
    Stack s;
    stack_init(&s);

    StackItem init_item = {STACK_DOLLAR};
    stack_push(&s, init_item);

    //TODO: získat první token od lexeru (Vojta??)
    Symbol input = DOLLAR; //zatím na tvrdo
    bool success = false;

    while(!success){
        StackItem top_item = stack_top_terminal(&s);
        Symbol top_term = (Symbol)top_item.type;

        Action action = precedence_table[top_term][input];

        switch(action){
            case S:{
                StackItem stop_item = {STACK_STOP};
                stack_push(&s, stop_item);
                StackItem input_item = {(StackSymbol)input};
                stack_push(&s, input_item);

                //načtení dalšího tokenu do lexeru
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
                    //TODO: gen_push_hodnoty(handle[0]);
                
                } //pravidlo E-> E + E (-,*, /, ==, ...)
                else if(handle_len == 3 && handle[0].type == STACK_NON_TERM &&
                        (handle[1].type == STACK_PLUS_MINUS_BIN || handle[1].type == STACK_MUL_DIV ||handle[1].type == STACK_RELATIONAL) &&
                        handle[2].type == STACK_NON_TERM){

                    rule_matched = true;
                    //TODO: gen_aritmetika(handle[1].type);
                
                } //pravidlo E -> (E)
                else if(handle_len == 3 &&
                        handle[0].type == STACK_R_PAR &&
                        handle[1].type == STACK_NON_TERM &&
                        handle[2].type == STACK_L_PAR){
                    
                    rule_matched = true;         
                
                }//pravidlo E -> -E (unární operátor)
                else if(handle_len == 2 &&
                        handle[0].type == STACK_NON_TERM &&
                        handle[1].type == STACK_PLUS_MINUS_UN){
                    
                    rule_matched = true;
                    //TODO: gen_unarni();
                }

                if(!rule_matched){
                    stack_free(&s);
                    return EXPR_ERR_SYNTAX;
                }

                StackItem non_term = {STACK_NON_TERM};
                stack_push(&s, non_term);

                break;
                
            }

            case E:{
                StackItem input_item = {(StackSymbol)input};
                stack_push(&s, input_item);
                //načtení dalšího tokenu do lexeru
                break;
            }

            case X:{
                StackItem input_item = {(StackSymbol)input};
                if(top_term == DOLLAR && input_item.type == STACK_DOLLAR){
                    success = true;
                }else{
                    stack_free(&s);
                    return EXPR_ERR_SYNTAX;
                }
                break;
            }
                
        }
    }

    return EXPR_OK;
}
