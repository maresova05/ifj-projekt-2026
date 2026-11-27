#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    SYMTABLE_OK = 0,
    SYMTABLE_EXISTS = 1,
    SYMTABLE_ALLOC_ERR = 2,
    SYMTABLE_INVALID_ARG = 3
} SymTableResult;

typedef enum {
    SYMBOL_VAR,
    SYMBOL_FUNC
} SymbolType;

typedef enum {
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_UNIT,
    TYPE_ANY,
    TYPE_UNDEFINED
} DataType;

typedef struct {
    SymbolType symbol_type;
    DataType data_type;

    //metadata proměnných
    bool is_mutable;
    bool is_param;
    int scope_id;

    //metadata funkcí
    bool is_defined;
    bool has_explicit_return_type;
    bool is_main;
    bool is_builtin;
    DataType *param_types;
    size_t param_count;

} SymbolData;




typedef struct BSTNode *SymTable;

void symtable_init(SymTable *table);

SymTableResult symtable_insert(SymTable *table, const char *key, const SymbolData *data);

SymbolData *symtable_search(SymTable table, const char *key);

void symtable_dispose(SymTable *table);


SymbolData symtable_make_var(DataType type, bool is_mutable, bool is_param, int scope_id);

SymbolData symtable_make_func(DataType return_type, bool is_defined, bool has_explicit_return_type, bool is_main);

SymTableResult symtable_func_add_param(SymbolData *func_data, DataType param_type);

SymTableResult symtable_init_builtins(SymTable *global_table);


typedef struct ScopeFrame {
    SymTable table;
    int scope_id;
    struct ScopeFrame *prev;
} ScopeFrame;

typedef struct {
    ScopeFrame *top;
    int next_scope_id;
} SymTableStack;

void symtable_stack_init(SymTableStack *stack);

SymTableResult symtable_stack_push(SymTableStack *stack);

void symtable_stack_pop(SymTableStack *stack);

SymTableResult symtable_stack_insert(SymTableStack *stack, const char *key, const SymbolData *data);

SymbolData *symtable_stack_search(const SymTableStack *stack, const char *key);

SymbolData *symtable_stack_search_current(const SymTableStack *stack, const char *key);

void symtable_stack_dispose(SymTableStack *stack);



#endif