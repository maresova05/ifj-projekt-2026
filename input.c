/*
    IFJ a IAL projekt
    xbartlm00 Matěj Bártl
*/

#include <stdio.h>
#include "input.h"
#include "dstring.h"
#include "error.h"

static dynamic_string_t loaded_data;
static bool redirected_std = false;

void input_load(void){
    if(dstring_init(&loaded_data) == false){
        report_error(ERR_INTERNAL,0);
    }
    int c;
    while((c =getchar())!=EOF){
        if(dstring_append(&loaded_data,c) == false){
            report_error(ERR_INTERNAL,0);
        }
    }
    if(loaded_data.lenght == 0){
            return;
        }
    FILE *file = fmemopen(loaded_data.str,loaded_data.lenght,"r");
        if (file == NULL){
            report_error(ERR_INTERNAL,0);
        }
    // použití kvůli dvěma průchody parseru, scanner čte přes getchar, spoléhá se na glibc
    stdin = file;
    redirected_std = true;
}

void input_rewind(void){
    rewind(stdin);
}

void input_free(void){
    if(redirected_std == true){
        fclose(stdin); 
    }
    dstring_free(&loaded_data);
}