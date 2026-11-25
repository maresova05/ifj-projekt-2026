/*
    IFJ a IAL projekt
    xbartlm00 Matěj Bártl
*/

#ifndef GEN_BASE_H
#define GEN_BASE_H
#define GEN_MAIN "$main" // volá vždy stejné návěští, využití v gen_start a u funkce s @main 

/**
* @brief volá se jako první slouží k vypsání .IFJcode26 a DEFVAR GF@$expr_op1 DEFVAR GF@$expr_op2
*/
void gen_head(void);

/** 
* @brief volá se hned po gen_head() a vypíše CREATEFRAME, CALL a EXIT int@0, funkce s @main musí mít návěští z makra
*/
void gen_start(void);

#endif