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

static void dispose_node(BSTNode *node){
    if (node != NULL){
        dispose_node(node->left);
        dispose_node(node->right);

        free(node->key);
        free(node);
    }
}

void symtable_dispose(SymTable *table){
    if (table != NULL && *table != NULL){
        dispose_node(*table);
        *table = NULL;
    }
}

//pomocné funkce pro vytváření binárního stromu
//výška uzlu (ošetření pro NULL)
static int get_height(BSTNode *node){
    if (node == NULL) return 0;
    return node->height;
}

//výška levého podstromu - výška pravého podstromu
static int get_balance(BSTNode *node){
    if (node == NULL) return 0;
    return get_height(node->left) - get_height(node->right);
}

//získání maxima ze dvou čísel
static int max(int a, int b){
    if (a > b) return a;
    return b;
}

static BSTNode* rotate_right(BSTNode *node){
    //záloha ukazatelů
    BSTNode *node_left = node->left;
    BSTNode *node_left_right = node_left->right;

    //rotace
    node_left->right = node;
    node->left = node_left_right;

    //aktualizace výšek
    node->height = max(get_height(node->left), get_height(node->right)) + 1;
    node_left->height = max(get_height(node_left->left), get_height(node_left->right)) + 1;

    //vrácení ukazatele na nový vrchol části stromu
    return node_left;
}

static BSTNode* rotate_left(BSTNode *node){
    //záloha ukazatelů
    BSTNode *node_right = node->right;
    BSTNode *node_right_left = node_right->left;

    //rotace
    node_right->left = node;
    node->right = node_right_left;

    //aktualizace výšek
    node->height = max(get_height(node->left), get_height(node->right)) + 1;
    node_right->height = max(get_height(node_right->left), get_height(node_right->right)) + 1;

    //vrácení ukazatele na nový vrchol části stromu
    return node_right;
}

