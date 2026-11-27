
#include <stdio.h>
#include "../scanner.h"

int main(void){
    Token token;
    int x;
    while(1){
        x = get_next_token(&token);
        if(x != 0){
            fprintf(stderr,"chyba : %d\n",x);
            return x;
        }    
        fprintf(stderr, "%d ",token.type);
        if(token.type == TOKEN_EOF){
            fprintf(stderr,"\n");
            break;
        }
    }
    return 0;
}