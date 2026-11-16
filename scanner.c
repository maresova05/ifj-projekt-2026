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
    STATE_IN_STRING,
    STATE_IN_STRING_ESCAPE,
    STATE_IN_MULTILINE_STRING,
    STATE_MULTILINE_1,
    STATE_MULTILINE_2,


}LexerState;
static int indent_stack[100] = {0}; //zásobník
static int top_idx = 0; //vrchol zásobníku
static int pending_dedents = 0;

static bool is_line_start = true;
static int current_spaces = 0;

static bool has_buffered_token = false;
static Token buffered_token;





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



int get_raw_token(Token *token){    
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
                }else if (c == '\n'){
                    is_line_start = true;
                    current_spaces = 0;
                    continue;
                }else if (c == ' '){
                    if (is_line_start) current_spaces++;
                    continue;
                }else if (c == '\t'){
                    if (is_line_start) return 1; //lexikální chyba (tabulátor je pro odsazování zakázaný)
                    continue;
                }else if (isspace(c)){
                    continue;
                }
                
                else if (c == '+'){
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
                }else if (c == '"'){
                    if (!dstring_init(&word)) return 99;

                    int c2 = getchar();
                    if (c2 == '"'){
                        int c3 = getchar();
                        if (c3 == '"'){
                            state = STATE_IN_MULTILINE_STRING;
                        }else{
                            ungetc(c3, stdin);
                            token->type = TOKEN_STRING_LITERAL;
                            token->attribute.s_val = word.str;
                            return 0;
                        }
                    }else{
                        ungetc(c2, stdin);
                        state = STATE_IN_STRING;
                    }


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

            case STATE_IN_STRING:
                //konec řetězce
                if (c == '"'){
                    token->type = TOKEN_STRING_LITERAL;
                    token->attribute.s_val = word.str;
                    return 0;
                }else if (c == '\\'){
                    state = STATE_IN_STRING_ESCAPE;
                }else if (c == '\n' || c == EOF){
                    //neukončený řetězec
                    dstring_free(&word);
                    return 1; //lexikální chyba
                }
                else{
                    //obyčejný text
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        return 99; //interní chyba
                    }
                }
            break;

            case STATE_IN_STRING_ESCAPE:
                if (c == '"'){
                    if (!dstring_append(&word, '"')) return 99;
                }else if (c == '\\'){
                    if (!dstring_append(&word, '\\')) return 99;
                }else if (c == 'n'){
                    if (!dstring_append(&word, '\n')) return 99;
                }else if (c == 'r'){
                    if (!dstring_append(&word, '\r')) return 99;
                }else if (c == 't'){
                    if (!dstring_append(&word, '\t')) return 99;
                }else{
                    //ostatní escape sekvence jsou neplatné
                    dstring_free(&word);
                    return 1;
                }
                state = STATE_IN_STRING;

            break;

            case STATE_IN_MULTILINE_STRING:
                if (c == EOF){
                    dstring_free(&word);
                    return 1;
                }else if (c == '"'){
                    state = STATE_MULTILINE_1;
                }else{
                    if (!dstring_append(&word, c)) return 99;
                }
            break;

            case STATE_MULTILINE_1:
                if (c == EOF){
                    dstring_free(&word);
                    return 1;
                }else if (c == '"'){
                    state = STATE_MULTILINE_2;
                }else{
                    if (!dstring_append(&word, '"')) return 99;
                    if (!dstring_append(&word, c)) return 99;
                    state = STATE_IN_MULTILINE_STRING;
                }
            break;

            case STATE_MULTILINE_2:
                if (c == EOF){
                    dstring_free(&word);
                    return 1;
                }else if (c == '"'){
                    token->type = TOKEN_STRING_LITERAL;
                    token->attribute.s_val = word.str;
                    return 0;
                }else{
                    if (!dstring_append(&word, '"')) return 99;
                    if (!dstring_append(&word, '"')) return 99;
                    if (!dstring_append(&word, c)) return 99;
                    state = STATE_IN_MULTILINE_STRING;
                }
            break;
        }
    }
}

int get_next_token(Token *token){
    if (pending_dedents > 0){
        pending_dedents--;
        token->type = TOKEN_DEDENT;
        return 0;
    }

    if (has_buffered_token){
        *token = buffered_token;
        has_buffered_token = false;
        return 0;
    }

    int result = get_raw_token(&buffered_token);
    if (result != 0){
        return result;
    }

    if (buffered_token.type == TOKEN_EOF){
        if (top_idx > 0){
            pending_dedents = top_idx;
            top_idx = 0;

            has_buffered_token = true;
            pending_dedents--;
            token->type = TOKEN_DEDENT;
            return 0;
        }
        *token = buffered_token;
        return 0;
    }

    if (is_line_start){
        is_line_start = false;

        if (current_spaces > indent_stack[top_idx]){
            top_idx++;
            indent_stack[top_idx] = current_spaces;

            has_buffered_token = true;
            token->type = TOKEN_INDENT;
            return 0;
        }else if (current_spaces < indent_stack[top_idx]){
            int dedent_count = 0;
            while (top_idx > 0 && current_spaces < indent_stack[top_idx]){
                dedent_count++;
                top_idx--;
            }

            if (current_spaces != indent_stack[top_idx]){
                return 1; //lexikální chyba
            }

            if (dedent_count > 0){
                has_buffered_token = true;
                pending_dedents = dedent_count - 1;

                token->type = TOKEN_DEDENT;
                return 0;
            }
        }
    }
    *token = buffered_token;
    return 0;
}
