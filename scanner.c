#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "scanner.h"
#include "dstring.h"

typedef enum {
    STATE_START,
    STATE_IN_WORD,

}LexerState;

//rozpoznávání klíčových slov
TokenType resolve_keyword(const char* word){
    if (strcmp(word, "def") == 0) return TOKEN_DEF;
    if (strcmp(word, "do") == 0) return TOKEN_DO;
    if (strcmp(word, "if") == 0) return TOKEN_IF;
    if (strcmp(word, "else") == 0) return TOKEN_ELSE;
    if (strcmp(word, "then") == 0) return TOKEN_THEN;
    if (strcmp(word, "import") == 0) return TOKEN_IMPORT;
    if (strcmp(word, "return") == 0) return TOKEN_RETURN;
    if (strcmp(word, "val") == 0) return TOKEN_VAL;
    if (strcmp(word, "var") == 0) return TOKEN_VAR;
    if (strcmp(word, "while") == 0) return TOKEN_WHILE;

    //rozšíření BOOLTHEN
    if (strcmp(word, "true") == 0) return TOKEN_TRUE;
    if (strcmp(word, "false") == 0) return TOKEN_FALSE;
    if (strcmp(word, "and") == 0) return TOKEN_AND;
    if (strcmp(word, "or") == 0) return TOKEN_OR;
    if (strcmp(word, "not") == 0) return TOKEN_NOT;
    
    return TOKEN_IDENTIFIER;
}



int get_next_token(Token *token){
    LexerState state = STATE_START;
    int c;
    dynamic_string_t word;

    token->line = 1;

    while (1){
        c = getchar();

        switch (state)
        {
            case STATE_START:
                if (c == EOF){
                    token->type = TOKEN_EOF;
                    return 0;
                }else if (isspace(c)){
                    continue;
                }else if (c == '+'){
                    token->type = TOKEN_PLUS;
                    return 0;
                }else if (c == '-'){
                    token->type = TOKEN_MINUS;
                    return 0;
                }else if (c == '*'){
                    token->type = TOKEN_MUL;
                    return 0;
                }else if (isalpha(c) || c == '_'){
                    if (!dstring_init(&word)){
                        return 99; //interní chyba
                    }

                    dstring_append(&word, c);
                    
                    state = STATE_IN_WORD;
                }else {
                    return 1; //lexikální chyba
                }

                break;

            case STATE_IN_WORD:
                if (isalnum(c) || c == '_'){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        return 99; //interní chyba
                    }
                }else{
                    ungetc(c, stdin);
                    token->type = resolve_keyword(word.str);
                    token->attribute.s_val = word.str;
                    return 0;
                }
        }
    }
}