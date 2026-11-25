#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include "scanner.h"
#include "dstring.h"
#include "error.h"

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
    STATE_OR,
    STATE_AND,
    STATE_AT,
    STATE_DOT,
    STATE_ZERO,
    STATE_IN_NUMBER_UNDERSCORE,
    STATE_IN_DOUBLE_START,
    STATE_IN_DOUBLE_UNDERSCORE,
    STATE_IN_EXP_START,
    STATE_IN_EXP_SIGN,
    STATE_IN_EXP,
    STATE_IN_EXP_UNDERSCORE,


}LexerState;

static int indent_stack[100] = {0}; //zásobník
static int top_idx = 0; //vrchol zásobníku
static int pending_dedents = 0;

static bool is_line_start = true;
static bool counting_indent = true;
static bool saw_invalid_indent = false;
static bool line_has_token = false;
static bool in_paren_continuation = false;

static int current_spaces = 0;
static int logical_line_indent = 0;
static int paren_depth = 0;
static int current_line = 1;

static bool has_buffered_token = false;
static Token buffered_token;

static void free_token_if_allocated(Token *token){
    if (token->type == TOKEN_IDENTIFIER || token->type == TOKEN_STRING_LITERAL){
        free(token->attribute.s_val);
        token->attribute.s_val = NULL;
    }
}





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

    if (strcmp(word, "Int") == 0) return TOKEN_KW_INT;
    if (strcmp(word, "Double") == 0) return TOKEN_KW_DOUBLE;
    if (strcmp(word, "String") == 0) return TOKEN_KW_STRING;
    if (strcmp(word, "Unit") == 0) return TOKEN_KW_UNIT;

    //rozšíření BOOLTHEN
    if (strcmp(word, "Boolean") == 0) return TOKEN_KW_BOOLEAN;
    if (strcmp(word, "true") == 0) return TOKEN_TRUE;
    if (strcmp(word, "false") == 0) return TOKEN_FALSE;
    /*
    if (strcmp(word, "and") == 0) return TOKEN_AND;
    if (strcmp(word, "or") == 0) return TOKEN_OR;
    if (strcmp(word, "not") == 0) return TOKEN_NOT;
    */
    return TOKEN_IDENTIFIER;
}



int get_raw_token(Token *token){    
    LexerState state = STATE_START;
    int c;
    dynamic_string_t word;

    //token->line = 1;

    while (1){
        c = getchar();

        switch (state)
        {
            case STATE_START:
                if (c == EOF){
                    token->type = TOKEN_EOF;
                    token->line = current_line;
                    return 0;
                }else if (c == '\n'){
                    if (paren_depth > 0){
                        current_line++;
                        in_paren_continuation = true;
                        counting_indent = true;
                        current_spaces = 0;
                        saw_invalid_indent = false;
                        continue;
                    }
                    if (line_has_token){
                        token->type = TOKEN_EOL;
                        token->line = current_line;
                        current_line++;
                        is_line_start = true;
                        counting_indent = true;
                        current_spaces = 0;
                        saw_invalid_indent = false;
                        line_has_token = false;
                        return 0;
                    }
                    current_line++;
                    is_line_start = true;
                    counting_indent = true;
                    current_spaces = 0;
                    saw_invalid_indent = false;
                    continue;
                }else if (c == ' '){
                    if ((is_line_start || in_paren_continuation) && counting_indent){
                        current_spaces++;
                    } 
                    continue;
                }else if (c == '\t'){
                    if ((is_line_start || in_paren_continuation) && counting_indent){
                        saw_invalid_indent = true;
                    }
                    continue;
                }else if (c == '\r'){
                    continue;
                }
                /*
                else if (isspace(c)){
                    continue;
                }
                */
                token->line = current_line;
                if (c == '+'){
                    token->type = TOKEN_PLUS;
                    return 0;
                }else if (c == '-'){
                    token->type = TOKEN_MINUS;
                    return 0;
                }else if (c == '*'){
                    token->type = TOKEN_MUL;
                    return 0;
                }else if (c == '('){
                    token->type = TOKEN_LPAREN;
                    paren_depth++;
                    return 0;
                }else if (c == ')'){
                    token->type = TOKEN_RPAREN;
                    if (paren_depth > 0) paren_depth--;
                    return 0;
                }else if (c == ':'){
                    token->type = TOKEN_COLON;
                    return 0;
                }else if (c == ','){
                    token->type = TOKEN_COMMA;
                    return 0;
                }else if (c == ';'){
                    token->type = TOKEN_SEMICOLON;
                    return 0;
                }else if (c == '/'){
                    state = STATE_SLASH;
                }else if (c == '&'){
                    state = STATE_AND;
                }else if (c == '|'){
                    state = STATE_OR;
                }else if (c == '@'){
                    state = STATE_AT;
                }else if (c == '.'){
                    state = STATE_DOT;
                }else if (c == '='){
                    state = STATE_ASSIGN;
                }else if (c == '!'){
                    state = STATE_NOT;
                }else if (c == '<'){
                    state = STATE_LESS;
                }else if (c == '>'){
                    state = STATE_GREATER;
                }else if (c == '"'){
                    if (!dstring_init(&word)) report_error(ERR_INTERNAL, current_line);

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


                }else if (c == '0'){
                    if (!dstring_init(&word)){
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line);
                    }
                    state = STATE_ZERO;
                }else if (isdigit(c)){
                    if (!dstring_init(&word)){
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line);
                    }
                    state = STATE_IN_NUMBER;
                }else if (isalpha(c) || c == '_'){
                    if (!dstring_init(&word)){
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }

                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line);
                    }
                    
                    state = STATE_IN_WORD;
                }else {
                    report_error(ERR_LEXICAL, current_line); //lexikální chyba
                }

            break;

            case STATE_IN_WORD:
                if (isalnum(c) || c == '_'){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }
                }else{
                    ungetc(c, stdin);

                    if (strcmp(word.str, "_") == 0){
                        dstring_free(&word);
                        report_error(ERR_LEXICAL, current_line); //lexikální chyba
                    }

                    token->type = resolve_keyword(word.str);
                    if (token->type == TOKEN_IDENTIFIER){
                        token->attribute.s_val = word.str;
                    }else{
                        dstring_free(&word);
                        token->attribute.s_val = NULL;
                    }
                    
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
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }
                //převod na double
                }else if (c == '_'){
                    state = STATE_IN_NUMBER_UNDERSCORE;
                }else if (c == '.'){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }
                    state = STATE_IN_DOUBLE_START;
                }else if (c == 'e' || c == 'E'){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }
                    state = STATE_IN_EXP_START;
                }
                /*
                else if (c == 'd' || c == 'D'){
                    int next = getchar();
                    if (isalnum(next) || next == '_'){
                        dstring_free(&word);
                        report_error(ERR_LEXICAL, current_line);
                    }
                    ungetc(next, stdin);
                    token->type = TOKEN_DOUBLE_LITERAL;
                    token->attribute.d_val = strtod(word.str, NULL);
                    dstring_free(&word);
                    return 0;
                }
                */
                else if (isalnum(c)){
                    dstring_free(&word);
                    report_error(ERR_LEXICAL, current_line);
                }else{
                    ungetc(c, stdin);
                    token->type = TOKEN_INT_LITERAL;

                    //převod textu na integer
                    token->attribute.i_val = strtoll(word.str, NULL, 10);
                    dstring_free(&word);
                    return 0;
                }
            break;

            case STATE_IN_NUMBER_UNDERSCORE:
                if (isdigit(c)){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }
                    state = STATE_IN_NUMBER;
                }else{
                    dstring_free(&word);
                    report_error(ERR_LEXICAL, current_line);
                }
            break;

            case STATE_IN_DOUBLE_START:
                if (isdigit(c)){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }
                    state = STATE_IN_DOUBLE;
                }else{
                    dstring_free(&word);
                    report_error(ERR_LEXICAL, current_line);
                }
            break;


            case STATE_IN_DOUBLE:
                if (isdigit(c)){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }
                }else if (c == '_'){
                    state = STATE_IN_DOUBLE_UNDERSCORE;
                }else if (c == 'e' || c == 'E'){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }
                    state = STATE_IN_EXP_START;
                }else if (c == 'd' || c == 'D'){
                    int next = getchar();
                    if (isalnum(next) || next == '_'){
                        dstring_free(&word);
                        report_error(ERR_LEXICAL, current_line);
                    }
                    ungetc(next, stdin);
                    token->type = TOKEN_DOUBLE_LITERAL;
                    token->attribute.d_val = strtod(word.str, NULL);
                    dstring_free(&word);
                    return 0;
                }else if (isalpha(c)){
                    dstring_free(&word);
                    report_error(ERR_LEXICAL, current_line);
                }else{
                    ungetc(c, stdin);
                    token->type = TOKEN_DOUBLE_LITERAL;

                    //převod textu na double
                    token->attribute.d_val = strtod(word.str, NULL);
                    dstring_free(&word);
                    return 0;
                }

            break;

            case STATE_IN_DOUBLE_UNDERSCORE:
                if (isdigit(c)){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }
                    state = STATE_IN_DOUBLE;
                }else{
                    dstring_free(&word);
                    report_error(ERR_LEXICAL, current_line);
                }
            break;

            case STATE_IN_EXP_START:
                if (c == '+' || c == '-'){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }
                    state = STATE_IN_EXP_SIGN;
                }else if (isdigit(c)){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }
                    state = STATE_IN_EXP;
                }else{
                    dstring_free(&word);
                    report_error(ERR_LEXICAL, current_line);
                }
            break;

            case STATE_IN_EXP_SIGN:
                if (isdigit(c)){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }
                    state = STATE_IN_EXP;
                }else{
                    dstring_free(&word);
                    report_error(ERR_LEXICAL, current_line);
                }
            break;

            case STATE_IN_EXP:
                if (isdigit(c)){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }
                }else if (c == '_'){
                    state = STATE_IN_EXP_UNDERSCORE;
                }else if (c == 'd' || c == 'D'){
                     int next = getchar();
                    if (isalnum(next) || next == '_'){
                        dstring_free(&word);
                        report_error(ERR_LEXICAL, current_line);
                    }
                    ungetc(next, stdin);
                    token->type = TOKEN_DOUBLE_LITERAL;
                    token->attribute.d_val = strtod(word.str, NULL);
                    dstring_free(&word);
                    return 0;
                }else if (isalpha(c)){
                    dstring_free(&word);
                    report_error(ERR_LEXICAL, current_line);
                }else{
                    ungetc(c, stdin);
                    token->type = TOKEN_DOUBLE_LITERAL;
                    token->attribute.d_val = strtod(word.str, NULL);
                    dstring_free(&word);
                    return 0;
                }
            break;

            case STATE_IN_EXP_UNDERSCORE:
                if (isdigit(c)){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }
                    state = STATE_IN_EXP;
                }else{
                    dstring_free(&word);
                    report_error(ERR_LEXICAL, current_line);
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
                    bool has_newline = false;

                    while (nest_level > 0){
                        c = getchar();

                        if (c == EOF){
                            report_error(ERR_LEXICAL, current_line); //lexikální chyba (konec souboru byl nalezen dřív než konec řádku)
                        }
                        //vnoření blokového komentáře
                        if (c == '\n'){
                            current_line++;
                            has_newline = true;
                        }else if (c == '/'){
                            int c_next = getchar();
                            if (c_next == '*'){
                                nest_level++;
                            }else if (c_next != EOF){
                                ungetc(c_next, stdin);
                            }
                        }else if (c == '*'){
                            int c_next = getchar();
                            if (c_next == '/'){
                                nest_level--;
                            }else if (c_next != EOF){
                                ungetc(c_next, stdin);
                            }
                        }
                    }
                    if (has_newline){
                        while (1){
                            c = getchar();
                            if (c == ' ' || c == '\t' || c == '\r'){
                                continue;
                            }else if (c == '\n') {
                                ungetc(c, stdin);
                                break;
                            }else if (c == EOF) {
                                break;
                            }else{
                                report_error(ERR_LEXICAL, current_line);
                            }
                        }
                        
                    }else if (is_line_start || in_paren_continuation) {
                        counting_indent = false;
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
                    report_error(ERR_LEXICAL, current_line); //lexikální chyba
                }
                else{
                    //obyčejný text
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line); //interní chyba
                    }
                }
            break;

            case STATE_IN_STRING_ESCAPE:
                if (c == '"'){
                    if (!dstring_append(&word, '"')){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line);
                    }
                }else if (c == '\\'){
                    if (!dstring_append(&word, '\\')){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line);
                    }
                }else if (c == 'n'){
                    if (!dstring_append(&word, '\n')){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line);
                    }
                }else if (c == 'r'){
                    if (!dstring_append(&word, '\r')){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line);
                    }
                }else if (c == 't'){
                    if (!dstring_append(&word, '\t')){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line);
                    }
                }else{
                    //ostatní escape sekvence jsou neplatné
                    dstring_free(&word);
                    report_error(ERR_LEXICAL, current_line);
                }
                state = STATE_IN_STRING;

            break;

            case STATE_IN_MULTILINE_STRING:
                if (c == EOF){
                    dstring_free(&word);
                    report_error(ERR_LEXICAL, current_line);
                }else if (c == '"'){
                    state = STATE_MULTILINE_1;
                }else{
                    if (c == '\n') current_line++;
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line);
                    }
                }
            break;

            case STATE_MULTILINE_1:
                if (c == EOF){
                    dstring_free(&word);
                    report_error(ERR_LEXICAL, current_line);
                }else if (c == '"'){
                    state = STATE_MULTILINE_2;
                }else{
                    if (c == '\n') current_line++;
                    if (!dstring_append(&word, '"') || !dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line);
                    }
                    state = STATE_IN_MULTILINE_STRING;
                }
            break;

            case STATE_MULTILINE_2:
                if (c == EOF){
                    dstring_free(&word);
                    report_error(ERR_LEXICAL, current_line);
                }else if (c == '"'){
                    int next = getchar();
                    while (next == '"'){
                        if (!dstring_append(&word, '"')){
                            dstring_free(&word);
                            report_error(ERR_INTERNAL, current_line);
                        }
                        next = getchar();
                    }
                    if (next != EOF){
                        ungetc(next, stdin);
                    }
                    token->type = TOKEN_STRING_LITERAL;
                    token->attribute.s_val = word.str;
                    return 0;
                }else{
                    if (c == '\n') current_line++;
                    if (!dstring_append(&word, '"') || !dstring_append(&word, '"') || !dstring_append(&word, c)) {
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line);
                    }
                    state = STATE_IN_MULTILINE_STRING;
                }
            break;

            case STATE_AND:
                if (c == '&'){
                    token->type = TOKEN_AND;
                    return 0;
                }
                report_error(ERR_LEXICAL, current_line); //lexikální chyba

            case STATE_OR:
                if (c == '|'){
                    token->type = TOKEN_OR;
                    return 0;
                }
                report_error(ERR_LEXICAL, current_line); //lexikální chyba

            case STATE_AT:
                if (c == 'm'){
                    int c2 = getchar();
                    int c3 = getchar();
                    int c4 = getchar();
                    int c5 = getchar();

                    //kontrola, jestli se za @ nachází přesně "main"
                    if (c2 == 'a' && c3 == 'i' && c4 == 'n' && !(isalnum(c5) || c5 == '_')){
                        ungetc(c5, stdin);
                        token->type = TOKEN_MAIN;
                        return 0;
                    }
                }
                report_error(ERR_LEXICAL, current_line); //lexikální chyba

            case STATE_DOT:
                if (isdigit(c)){
                    if (!dstring_init(&word)) report_error(ERR_INTERNAL, current_line);
                    if (!dstring_append(&word, '0') || !dstring_append(&word, '.') || !dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line);
                    }
                    state = STATE_IN_DOUBLE;
                }else{
                    ungetc(c, stdin);
                    token->type = TOKEN_DOT;
                    return 0;
                }
            break;

            case STATE_ZERO:
                if (c == '.'){
                     if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line);
                    }
                    state = STATE_IN_DOUBLE_START;
                }else if (c == 'e' || c == 'E'){
                    if (!dstring_append(&word, c)){
                        dstring_free(&word);
                        report_error(ERR_INTERNAL, current_line);
                    }
                    state = STATE_IN_EXP_START;
                }
                /*
                else if (c == 'd' || c == 'D'){
                    int next = getchar();
                    if (isalnum(next) || next == '_'){
                        dstring_free(&word);
                        report_error(ERR_LEXICAL, current_line);
                    }
                    ungetc(next, stdin);
                    token->type = TOKEN_DOUBLE_LITERAL;
                    token->attribute.d_val = 0.0;
                    dstring_free(&word);
                    return 0;
                }
                */
                else if (isalnum(c) || c == '_'){
                    dstring_free(&word);
                    report_error(ERR_LEXICAL, current_line);
                }else{
                    ungetc(c, stdin);
                    token->type = TOKEN_INT_LITERAL;
                    token->attribute.i_val = 0;
                    dstring_free(&word);
                    return 0;
                }
            break;

        }
    }
}

int get_next_token(Token *token){
    if (pending_dedents > 0){
        pending_dedents--;
        token->type = TOKEN_DEDENT;
        token->line = buffered_token.line;
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

    if (buffered_token.type == TOKEN_EOL){
        *token = buffered_token;
        return 0;
    }

    if (buffered_token.type == TOKEN_EOF){
        if (line_has_token && paren_depth == 0){
            line_has_token = false;
            is_line_start = true;
            counting_indent = true;
            current_spaces = 0;
            saw_invalid_indent = false;

            if (top_idx > 0){
                pending_dedents = top_idx;
                top_idx = 0;
            }

            has_buffered_token = true;
            token->type = TOKEN_EOL;
            token->line = buffered_token.line;
            return 0;
        }
        if (top_idx > 0){
            pending_dedents = top_idx - 1;
            top_idx = 0;
            has_buffered_token = true;
            token->type = TOKEN_DEDENT;
            token->line = buffered_token.line;
            return 0;
        }
        *token = buffered_token;
        return 0;
    }

    line_has_token = true;

    if (saw_invalid_indent){
        free_token_if_allocated(&buffered_token);
        report_error(ERR_LEXICAL, current_line);
    }

    if (in_paren_continuation){
        in_paren_continuation = false;
        bool bad_indent = (buffered_token.type == TOKEN_RPAREN) ? (current_spaces < logical_line_indent) : (current_spaces<= logical_line_indent);
        if (bad_indent) {
            free_token_if_allocated(&buffered_token);
            report_error(ERR_LEXICAL, current_line);
        }
        *token = buffered_token;
        return 0;
    }

    if (is_line_start){
        is_line_start = false;
        logical_line_indent = current_spaces;

        if (current_spaces > indent_stack[top_idx]){
            if (top_idx >= 99){
                free_token_if_allocated(&buffered_token);
                report_error(ERR_INTERNAL, current_line);
            }
            top_idx++;
            indent_stack[top_idx] = current_spaces;

            has_buffered_token = true;
            token->type = TOKEN_INDENT;
            token->line = buffered_token.line;
            return 0;
        }else if (current_spaces < indent_stack[top_idx]){
            int dedent_count = 0;
            while (top_idx > 0 && current_spaces < indent_stack[top_idx]){
                dedent_count++;
                top_idx--;
            }

            if (current_spaces != indent_stack[top_idx]){
                free_token_if_allocated(&buffered_token);
                report_error(ERR_LEXICAL, current_line); //lexikální chyba
            }

            if (dedent_count > 0){
                has_buffered_token = true;
                pending_dedents = dedent_count - 1;

                token->type = TOKEN_DEDENT;
                token->line = buffered_token.line;
                return 0;
            }
        }
    }
    *token = buffered_token;
    return 0;
}

//AI: optimalizace pro testování
void scanner_reset(void) {
    // Pokud v bufferu zůstal viset alokovaný token po předchozí chybě, uvolníme ho
    if (has_buffered_token && 
       (buffered_token.type == TOKEN_IDENTIFIER || buffered_token.type == TOKEN_STRING_LITERAL)) {
        free(buffered_token.attribute.s_val);
    }
    top_idx = 0;
    indent_stack[0] = 0;
    pending_dedents = 0;
    is_line_start = true;
    current_spaces = 0;
    has_buffered_token = false;
    counting_indent = true;
    saw_invalid_indent = false;
    line_has_token = false;
    in_paren_continuation = false;
    logical_line_indent = 0;
    paren_depth = 0;
    current_line = 1;
}