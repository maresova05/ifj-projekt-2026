#ifndef DSTRING_H
#define DSTRING_H

#include <stdbool.h>
/*
 * @brief Struktura dynamicky alokovaného řetězce
 */
typedef struct {
  char *str;      // ukazatel na text
  int lenght;     // počet znaků
  int alloc_size; // velikost alokované paměti
} dynamic_string_t;

/*
 * @brief Inicializuje prázdný dynamický řetězec
 */
bool dstring_init(dynamic_string_t *s);

/*
 * @brief Přidá jeden znak na konec dynamicky alokovaného řetězce
 */
bool dstring_append(dynamic_string_t *s, char c);

/*
 * @brief Uvolní paměť dynamicky alokovaného řetězce a vynuluje položky
 * struktury
 */
void dstring_free(dynamic_string_t *s);

#endif