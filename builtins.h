/*
    IFJ a IAL projekt
    xbartlm00 Matěj Bártl
*/

#ifndef BUILTINS_H
#define BUILTINS_H
#include "scanner.h"
#include "symtable.h"

/**
 * @brief vypíše int@, float@, string@ nebo LF@jmeno$cislo, konec řádku nevypisuje
 * @param token token
 * @param sym záznam proměnné z tabulky symbolů (výsledek symtable_stack_search)
*/
void gen_operand(Token *token,SymbolData *sym);

/**
 * @brief Vypíše WRITE s operandem z funkce gen_operand, nebo při typu Unit řetězec se závorkami. Konec řádku vypisuje.
 * @param token token
 * @param sym záznam proměnné z tabulky symbolů (výsledek symtable_stack_search)
 */
void gen_print(Token *token,SymbolData *sym);

#endif