#include <stdlib.h>
#include <string.h>
#include "symtable.h"

void symtable_init(SymTable *table){
    *table = NULL;  
}

BSTNode* symtable_search(SymTable table, const char *key){
    BSTNode *current = table;

    while (current != NULL){
        if (strcmp(key, current->key) < 0){
            current = current->left;
        }else if (strcmp(key, current->key) > 0){
            current = current->right;
        }else{
            return current;
        }
    }
    return NULL;
}