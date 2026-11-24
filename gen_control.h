#ifndef GEN_CONTROL_H
#define GEN_CONTROL_H

/**
* @brief vrátí nové číslo pro LABEL jedné konstrukce
* @return číslo použité v názvech LABEL
*/
int gen_new_label_id(void);

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

#endif