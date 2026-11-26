#ifndef GEN_CONTROL_H
#define GEN_CONTROL_H

/**
* @brief vrátí nové číslo pro LABEL jedné konstrukce
* @return číslo použité v názvech LABEL
*/
int gen_new_label_id(void);

/**
* @brief vypíše návěští GEN_MAIN pro funkci s anotací @main
*/
void gen_main_label(void);

/**
* @brief čeká na bool na vrcholu zásobníku a skočí na větev else, když je nepravda
* @param id číslo z konstrukce gen_new_label_id
*/
void gen_if_start(int id);

/**
* @brief skočí za konec if a vypíše LABEL větve else
* @param id číslo konstrukce
*/
void gen_if_else(int id);

/**
* @brief vypíše koncový LABEL if
* @param id číslo konstrukce
*/
void gen_if_end(int id);

/**
* @brief vypíše LABEL začátku cyklu
* @param id číslo konstrukce
*/
void gen_while_start(int id);

/**
* @brief čeká na bool na vrcholu zásobníku, při nepravdě skočí na konec
* @param id číslo konstrukce
*/
void gen_while_cond(int id);

/**
* @brief skočí zpět na začátek cyklu a vypíše koncový LABEL
* @param id číslo konstrukce
*/
void gen_while_end(int id);

/**
* @brief vypíše LABEL funkce, vytvoří lokální rámec a založí %retval s hodnotou nil
* @param name jméno funkce
*/
void gen_func_start(const char *name);

/**
* @brief zruší lokální rámec a vrátí se k volajícímu, i pro prázdný return
*/
void gen_func_end(void);

/**
* @brief převede parametr z %paramN na proměnnou name$scope_id
* @param name jméno parametru
* @param scope_id číslo rozsahu z tabulky symbolů
* @param index pořadí parametru zleva
*/
void gen_func_param(const char *name, int scope_id, int index);

/**
* @brief uloží hodnotu ze zásobníku do %retval a vrátí se z funkce
*/
void gen_return_value(void);

/**
* @brief vypíše POPS GF@$expr_op1 a hodnotu ze zásobníku zahodí
*/
void gen_expr_discard(void);

/**
* @brief vypíše POPS LF@%retval a hodnotu uloží jako návratovou
*/
void gen_expr_to_retval(void);

#endif