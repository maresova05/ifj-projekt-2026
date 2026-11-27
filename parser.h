/**
 * @file parser.h
 * @brief Rozhraní syntaktického analyzátoru 
 * Projekt: Implementace překladače imperativního jazyka IFJ26
 *
 * @author Jan Dobrovolný (xdobroj00)
 */

#ifndef PARSER_H
#define PARSER_H

/**
* @brief syntaktická analýza programu rekurzivním sestupem
* @return vrací 0 při úspěchu, při syntaktické chybě ukončí překlad s kódem 2
*/
int parse_program(void);

#endif