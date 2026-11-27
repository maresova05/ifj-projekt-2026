/*
    IFJ a IAL projekt
    xbartlm00 Matěj Bártl
*/

#ifndef ERROR_H
#define ERROR_H

typedef enum{
    ERR_LEXICAL = 1,               // lexikální chyba
    ERR_SYNTAX = 2,                // syntaktická chyba
    ERR_UNDEFINED_SYMBOL = 3,      // nedefinovaná funkce, proměnná nebo typ, chybějící @main
    ERR_REDEFINITION = 4,          // redefinice
    ERR_ARGUMENTS = 5,             // špatný počet nebo typ argumentů, parametr u @main
    ERR_TYPE = 6,                  // typová kompatibilita nebo odvození typu
    ERR_IMMUTABLE = 7,             // zápis do nemodifikovatelné proměnné nebo formálního parametru funkce
    ERR_OTHER_SEMANTIC = 10,       // ostatní sémantické chyby(nepovolený return, nevyčerpávající match a nulový krok rozsahu)
    ERR_INTERNAL = 99              // interní chyba překladače
}Error_code;

/**
* @brief ukončí program s hodnotou kódu a vypíše hlášení o chybě na stderr
* @param Error_code err, kód chyby, která vznikla 
* @param int line, číslo řádku na kterém chyba vznikla, pokud zapsaná hodnota čísla řádku menší jak 1, tak se řádek nevypíše 
*/
__attribute__((noreturn))
void report_error(Error_code err,int line);

#endif