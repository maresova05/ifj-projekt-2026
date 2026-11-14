#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "scanner.h"
#include "dstring.h"

typedef enum {
    STATE_START,
    STATE_IN_WORD,
    STATE_ASSIGN,
    STATE_NOT,
    STATE_LESS,
    STATE_GREATER,
    STATE_IN_NUMBER,
    STATE_IN_DOUBLE,
    STATE_SLASH,

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
                }else if (c == '/'){
                    state = STATE_SLASH;
                }else if (c == '='){
                    state = STATE_ASSIGN;
                }else if (c == '!'){
                    state = STATE_NOT;
                }else if (c == '<'){
                    state = STATE_LESS;
                }else if (c == '>'){
                    state = STATE_GREATER;
                }else if (isdigit(c)){
                    if (!dstring_init(&word)){
                        return 99; //interní chyba
                    }
                    dstring_append(&word, c);
                    state = STATE_IN_NUMBER;
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
            break;

            case STATE_ASSIGN:
                if (c == '='){
                    token->type = TOKEN_EQ;
                }else{
                    ungetc(c, stdin);
                    token->type = TOKEN_ASSIGN;
                }
                return 0;

            case STATE_NOT:
                if (c == '='){
                    token->type = TOKEN_NEQ;
                }else{
                    ungetc(c, stdin);
                    token->type = TOKEN_NOT;
                }
                return 0;

            case STATE_LESS:
                if (c == '='){
                    token->type = TOKEN_LESS_EQ;
                }else{
                    ungetc(c, stdin);
                    token->type = TOKEN_LESS;
                }
                return 0;

            case STATE_GREATER:
                if (c == '='){
                    token->type = TOKEN_GREATER_EQ;
                }else{
                    ungetc(c, stdin);
                    token->type = TOKEN_GREATER;
                }
                return 0;

            case STATE_IN_NUMBER:
                //přidání čísla
                if (isdigit(c)){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        return 99; //interní chyba
                    }
                //převod na double
                }else if (c == '.'){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        return 99; //interní chyba
                    }
                    state = STATE_IN_DOUBLE;
                //ukončení čísla
                }else{
                    ungetc(c, stdin);
                    token->type = TOKEN_INT_LITERAL;

                    //převod textu na integer
                    token->attribute.i_val = strtoll(word.str, NULL, 10);
                    dstring_free(&word);
                    return 0;
                }
            break;
                

            case STATE_IN_DOUBLE:
                if (isdigit(c)){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        return 99; //interní chyba
                    }
                }else{
                    ungetc(c, stdin);
                    token->type = TOKEN_DOUBLE_LITERAL;

                    //převod textu na double
                    token->attribute.d_val = strtod(word.str, NULL);
                    dstring_free(&word);
                    return 0;
                }
            break;

            case STATE_SLASH:
                //řádkový komentář
                if (c == '/'){
                    while (1){
                        c = getchar();
                        if (c == '\n' || c == EOF){
                            ungetc(c, stdin);
                            break;
                        }
                    }
                    state = STATE_START;
                //začátek blokového komentáře
                }else if (c == '*'){
                    int nest_level = 1; //počítadlo vnoření blokových komentářů

                    while (nest_level > 0){
                        c = getchar();

                        if (c == EOF){
                            return 1; //lexikální chyba (konec souboru byl nalezen dřív než konec řádku)
                        }
                        //vnoření blokového komentáře
                        if (c == '/'){
                            int c_next = getchar();
                            if (c_next == '*'){
                                nest_level++;
                            }else{
                                ungetc(c_next, stdin);
                            }
                        }else if (c == '*'){
                            int c_next = getchar();
                            if (c_next == '/'){
                                nest_level--;
                            }else{
                                ungetc(c_next, stdin);
                            }
                        }

                    }
                    state = STATE_START;
                //dělení
                }else{
                    ungetc(c, stdin);
                    token->type = TOKEN_DIV;
                    return 0;
                }
            break;
        }
    }
}
