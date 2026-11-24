#include <stdlib.h>
#include "dstring.h"

#define INIT_STRING_SIZE 8

//inicializace prázdného řetězce
bool dstring_init(dynamic_string_t *s){
    s->str = (char *)malloc(INIT_STRING_SIZE * sizeof(char));

    if (s->str == NULL){
        return false; //chyba alokace paměti
    }

    s->str[0] = '\0';
    s->lenght = 0;
    s->alloc_size = INIT_STRING_SIZE;

    return true;
}

//přidání prázdného znaku na konec řetězce
bool dstring_append(dynamic_string_t *s, char c){
    //kontrola velikosti paměti a případná realokace
    if (s->lenght + 1 >= s->alloc_size){
        int new_size = s->alloc_size * 2;
        char *new_str = (char *)realloc(s->str, new_size * sizeof(char));
    
        if (new_str == NULL){
            return false; //chyba realokace paměti
        }

        s->str = new_str;
        s->alloc_size = new_size;
    }

    //přidání znaku na konec
    s->str[s->lenght] = c;
    s->lenght++;
    s->str[s->lenght] = '\0';

    return true;
}

//uvolnění paměti
void dstring_free(dynamic_string_t *s){
    if(s->str != NULL){
        free(s->str);
        s->str = NULL;
    }
    s->lenght = 0;
    s->alloc_size = 0;

}

