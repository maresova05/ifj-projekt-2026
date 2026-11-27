/*
    IFJ a IAL projekt
    xbartlm00 Matěj Bártl
*/

#ifndef GEN_BASE_H
#define GEN_BASE_H
#define GEN_MAIN "$$main" // jméno návěští, využití v gen_start a u funkce s @main 

/**
* @brief volá se jako první slouží k vypsání .IFJcode26 a DEFVAR GF@$expr_op1 DEFVAR GF@$expr_op2
*/
void gen_head(void);

/** 
* @brief volá se hned po gen_head() a vypíše CREATEFRAME, CALL a EXIT int@0, funkce s @main musí mít návěští z makra
*/
void gen_start(void);

/**
 * @brief veškerý výstup z IFJcode26 jde přes tuto funkci místo printf/putchar, mimo cyklus vypisuje rovnou na stdout, uvnitř cyklu ukládá do paměti, chybová hlášení na stderr přes ni nejdou
 * @param format vypisovaný formát
 */
void gen_emit(const char *format, ...);

/**
 * @brief mimo cyklus vypíše DEFVAR LF@jmeno$cislo hned, uvnitř cyklu si ho schová a vypíše ho až před začátkem vnějšího cyklu, aby se neprovedl při každé iteraci 
 * @param name jméno proměnné
 * @param scope_id číslo bloku
 */
void gen_defvar(const char *name,int scope_id);

/**
 * @brief volá se na začátku while, před vypsáním návěští začátku cyklu, u vnořeného cyklu jen zvýší hloubku
 */
void gen_loop_begin(void);

/**
 * @brief volá se na konci while, za vypsáním návěští konce cyklu, při opuštění vnějšího cyklu vypíše na stdout nejdřív všechny schované DEFVAR, pak schovaný kód cyklu, a vrátí se k přímému výpisu
 */
void gen_loop_end(void);

#endif