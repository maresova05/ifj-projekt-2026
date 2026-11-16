#ifndef SYMTABLE_H
#define SYMTABLE_H

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
    TYPE_UNDEFINED,
} DataType;

typedef struct BSTNode{
    char *key;
    SymbolType symbol_type;
    DataType data_type;

    int height;

    struct BSTNode *left;
    struct BSTNode *right;
}BSTNode;

typedef BSTNode* SymTable;

void symtable_init(SymTable *table);

bool symtable_insert(SymTable *table, const char *key, SymbolType sym_type, DataType data_type);

BSTNode* symtable_search(SymTable table, const char *key);

void symtable_dispose(SymTable *table);





#endif