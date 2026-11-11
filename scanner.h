#ifdef SCANNER_H
#define SCANNER_H


typedef enum {
    //1. Speciální tokeny
    TOKEN_EOF, //konec souboru
    TOKEN_EOL, //konec řádku
    TOKEN_INDENT, //zvětšení odsazení
    TOKEN_DEDENT, //zmenšení odsazení

    //2. Literály a identifikátory
    TOKEN_IDENTIFIER, //např. promenna_nazev
    TOKEN_INT_LITERAL, //např. 86
    TOKEN_DOUBLE_LITERAL, //např. 3.14
    TOKEN_STRING_LITERAL, //např. "Jak se mas"

    //3. Klíčová slova
    TOKEN_DEF,
    TOKEN_DO,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_THEN,
    TOKEN_IMPORT,
    TOKEN_RETURN,
    TOKEN_VAL,
    TOKEN_VAR,
    WHILE,

    //3,5. Klíčová slova (rozšíření)
    TOKEN_TRUE, //BOOLTHEN
    TOKEN_FALSE, //BOOLTHEN

    //4. Operátory a speciální znaky
    TOKEN_PLUS, // +
    TOKEN_MINUS, // -
    TOKEN_MUL, // *
    TOKEN_DIV, // /
    TOKEN_ASSIGN, // =
    TOKEN_EQ, // ==
    TOKEN_NEQ, // !=
    TOKEN_LESS, // <
    TOKEN_LESS_EQ, // <=
    TOKEN_GREATER, // >
    TOKEN_GREATER_EQ, // >=
    TOKEN_LPAREN, // (
    TOKEN_RPAERN, // )
    TOKEN_COLON, // :
    TOKEN_COMMA, // ,
    
    //4,5. Operátory a speciální znaky (rozšíření)
    TOKEN_AND, // && (BOOLTHEN)
    TOKEN_OR, // || (BOOLTHEN)
    TOKEN_NOT // ! (BOOLTHEN)


} TokenType;

typedef struct {
    TokenType type; //typ tokenu
    int line; //řádek, na kterém se token nachází

    union {
        long long i_val; //int
        double d_val; //double
        char* s_val; //string a identifikátor
    } attribute;
} Token;

int get_next_token(Token *token);








#endif