#ifndef DSTRING_H
#define DSTRING_H

#include <stdbool.h>

typedef struct
{
    char *str; //ukazatel na text
    int lenght; //počet znaků
    int alloc_size; //velikost alokované paměti
} dynamic_string_t;

bool dstring_init(dynamic_string_t *s);
bool dstring_append(dynamic_string_t *s, char c);
void dstring_free(dynamic_string_t *s);

#endif