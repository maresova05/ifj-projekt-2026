/*
 * Soubor: symtable.h
 * Projekt: Překladač jazyka IFJ26 (IFJ/IAL 2026/2027)
 * Autoři:
 * - Vojtěch Koubek, xkoubev00
 */
#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>
#include <stddef.h>
 
/*
 * @brief Návratové kódy operací nad tabulkou symbolů
 */
typedef enum {
  SYMTABLE_OK = 0,
  SYMTABLE_EXISTS = 1,
  SYMTABLE_ALLOC_ERR = 2,
  SYMTABLE_INVALID_ARG = 3
} SymTableResult;

/*
 * @brief Druh symbolu (proměnná nebo funkce)
 */
typedef enum { SYMBOL_VAR, SYMBOL_FUNC } SymbolType;

/*
 * @brief Datové typy jazyka IFJ26 + rozšíření BOOLTHEN a pomocné typy
 */
typedef enum {
  TYPE_INT,
  TYPE_DOUBLE,
  TYPE_STRING,
  TYPE_BOOL,
  TYPE_UNIT,
  TYPE_ANY,
  TYPE_UNDEFINED
} DataType;

/*
 * @brief Metadata symbolu (proměnná nebo funkce)
 */
typedef struct {
  SymbolType symbol_type;
  DataType data_type;

  // metadata proměnných
  bool is_mutable;
  bool is_param;
  int scope_id;

  // metadata funkcí
  bool is_defined;
  bool has_explicit_return_type;
  bool is_main;
  bool is_builtin;
  DataType *param_types;
  size_t param_count;

} SymbolData;

/*
 * @brief Ukazatel na kořen tabulky symbolů (skryté)
 */
typedef struct BSTNode *SymTable;

/*
 * @brief Inicializuje prázdnou tabulku symbolů
 */
void symtable_init(SymTable *table);

/*
 * @brief Vloží nový symbol do tabulky symbolů
 */
SymTableResult symtable_insert(SymTable *table, const char *key,
                               const SymbolData *data);

/*
 * @brief Vyhledá symbol v tabulce na základě jeho klíče
 */
SymbolData *symtable_search(SymTable table, const char *key);

/*
 * @brief Uvolní všechny uzly tabulky symbolů včetně jejich obsahu
 */
void symtable_dispose(SymTable *table);

/*
 * @brief Vytvoří inicializovanou strukturu SymbolData pro proměnnou
 */
SymbolData symtable_make_var(DataType type, bool is_mutable, bool is_param,
                             int scope_id);

/*
 * @brief Vytvoří inicializovanou strukturu SymbolData pro funkci
 */
SymbolData symtable_make_func(DataType return_type, bool is_defined,
                              bool has_explicit_return_type, bool is_main);

/*
 * @brief Přidá datový typ dalšího parametru do metadat funkce
 */
SymTableResult symtable_func_add_param(SymbolData *func_data,
                                       DataType param_type);

/*
 * @brief Přidá všechny vestavěné funkce do globální tabulky symbolů
 */
SymTableResult symtable_init_builtins(SymTable *global_table);

/*
 * @brief Jeden rámec na zásobníku tabulek symbolů
 */
typedef struct ScopeFrame {
  SymTable table;
  int scope_id;
  struct ScopeFrame *prev;
} ScopeFrame;

/*
 * @brief Zásobník tabulek symbolů pro správu vnoření bloků a shadowing
 * proměnných
 */
typedef struct {
  ScopeFrame *top;
  int next_scope_id;
} SymTableStack;

/*
 * @brief Inicializuje prázdný zásobník tabulek symbolů
 */
void symtable_stack_init(SymTableStack *stack);

/*
 * @brief Vytvoří nový prázdný rámec a vloží jej na vrchol zásobníku
 */
SymTableResult symtable_stack_push(SymTableStack *stack);

/*
 * @brief Odstraní vrchní rámec ze zásobníku
 */
void symtable_stack_pop(SymTableStack *stack);

/*
 * @brief Vloží symbol do vrchního rámce zásobníku a u proměnných automaticky
 * doplní scope_id aktuálního rámce
 */
SymTableResult symtable_stack_insert(SymTableStack *stack, const char *key,
                                     const SymbolData *data);

/*
 * @brief Vyhledá symbol od nejvnitřnějšího rámce
 */
SymbolData *symtable_stack_search(const SymTableStack *stack, const char *key);

/*
 * @brief Vyhledá symbol pouze v nejvrchnějším rámci
 */
SymbolData *symtable_stack_search_current(const SymTableStack *stack,
                                          const char *key);

/*
 * @brief Uvolní všechny rámce ze zásobníku a vynuluje scope_id
 */
void symtable_stack_dispose(SymTableStack *stack);

#endif