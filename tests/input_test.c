
#include <stdio.h>
#include "../input.h"

int main(void){
    input_load();
    int c;
    while ((c = getchar()) != EOF){
        putchar(c);
    }
    input_rewind();
    while ((c = getchar()) != EOF){
        putchar(c);
    }
    input_free();
}