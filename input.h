/*
    IFJ a IAL projekt
    xbartlm00 Matěj Bártl
*/

#ifndef INPUT_H
#define INPUT_H

/**
 * @brief načte celý standardní vstup do paměti a přesměruje stdin, aby se z té paměti četlo, volá se jako úplně první věc v main, prázdný vstup nechá stdin beze změny
 */
void input_load(void);

/**
 * @brief vrátí čtení na začátek
 */
void input_rewind(void);

/**
 * @brief uvolní data, volá se na konci main
 */
void input_free(void);

#endif