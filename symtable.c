#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "symtable.h"

typedef struct BSTNode{
    char *key;
    SymbolData data;

    int height;

    struct BSTNode *left;
    struct BSTNode *right;
}BSTNode;



void symtable_init(SymTable *table){
    if (table == NULL) return;
    *table = NULL;  
}

SymbolData* symtable_search(SymTable table, const char *key){
    if (key == NULL) return NULL;
    BSTNode *current = table;

    

    while (current != NULL){
        int cmp = strcmp(key, current->key);
        if (cmp < 0){
            current = current->left;
        }else if (cmp > 0){
            current = current->right;
        }else{
            return &current->data;
        }
    }
    return NULL;
}

static void dispose_node(BSTNode *node){
    if (node != NULL){
        dispose_node(node->left);
        dispose_node(node->right);

        free(node->data.param_types);
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

static BSTNode* insert_node(BSTNode *node, const char *key, const SymbolData *data, SymTableResult *status){
    if (node == NULL){
        BSTNode *new_node = (BSTNode*)malloc(sizeof(BSTNode));
        if (new_node == NULL){
            *status = SYMTABLE_ALLOC_ERR;
            return NULL; //chyba alokace
        }

        new_node->key = malloc(strlen(key) + 1);
        if (new_node->key == NULL){
            free(new_node);
            *status = SYMTABLE_ALLOC_ERR;
            return NULL; //chyba alokace
        }
        strcpy(new_node->key, key);

        new_node->data = *data;
        if (data->param_count > 0 && data->param_types != NULL){
            new_node->data.param_types = malloc(data->param_count * sizeof(DataType));
            if (new_node->data.param_types == NULL){
                free(new_node->key);
                free(new_node);
                *status = SYMTABLE_ALLOC_ERR;
                return NULL;
            }
            memcpy(new_node->data.param_types, data->param_types, data->param_count * sizeof(DataType));
        }else{
            new_node->data.param_types = NULL;
            new_node->data.param_count = 0;
        }

        new_node->height = 1;
        new_node->left = NULL;
        new_node->right = NULL;

        *status = SYMTABLE_OK;
        return new_node;
    }

    int cmp = strcmp(key, node->key);
    if(cmp < 0){
        node->left = insert_node(node->left, key, data, status);
    }else if(cmp > 0){
        node->right = insert_node(node->right, key, data, status);
    }else{
        *status = SYMTABLE_EXISTS;
        return node;
    }
    if (*status != SYMTABLE_OK) return node;

    node->height = max(get_height(node->left), get_height(node->right)) + 1;

    int balance = get_balance(node);
    
    if (balance > 1 && get_balance(node->left) >= 0){ //LL
        return rotate_right(node);
    }else if (balance < -1 && get_balance(node->right) <= 0){ //RR
        return rotate_left(node);
    }else if (balance > 1 && get_balance(node->left) < 0){ //LR
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }else if (balance < -1 && get_balance(node->right) > 0){ //RL
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node;
    
}

SymTableResult symtable_insert(SymTable *table, const char *key, const SymbolData *data){
    if (table == NULL || key == NULL || data == NULL) return SYMTABLE_INVALID_ARG;
    SymTableResult status = SYMTABLE_OK;
    *table = insert_node(*table, key, data, &status);
    return status;
}



SymbolData symtable_make_var(DataType type, bool is_mutable, bool is_param, int scope_id){
    SymbolData d;
    memset(&d, 0, sizeof(d));
    d.symbol_type = SYMBOL_VAR;
    d.data_type = type;
    d.is_mutable = is_mutable;
    d.is_param = is_param;
    d.scope_id = scope_id;
    return d;
}

SymbolData symtable_make_func(DataType return_type, bool is_defined, bool has_explicit_return_type, bool is_main){
    SymbolData d;
    memset(&d, 0, sizeof(d));
    d.symbol_type = SYMBOL_FUNC;
    d.data_type = return_type;
    d.is_defined = is_defined;
    d.has_explicit_return_type = has_explicit_return_type;
    d.is_main = is_main;
    return d;
}

SymTableResult symtable_func_add_param(SymbolData *func_data, DataType param_type){
    if (func_data == NULL || func_data->symbol_type != SYMBOL_FUNC){
        return SYMTABLE_INVALID_ARG;
    }

    size_t new_count = func_data->param_count + 1;
    DataType *new_arr = realloc(func_data->param_types, new_count * sizeof(DataType));
    if (new_arr == NULL) return SYMTABLE_ALLOC_ERR;

    new_arr[func_data->param_count] = param_type;
    func_data->param_types = new_arr;
    func_data->param_count = new_count;
    return SYMTABLE_OK;
}

static SymTableResult register_builtin(SymTable *table, const char *name, DataType ret_type, const DataType *params, size_t param_count){
    SymbolData fn = symtable_make_func(ret_type, true, true, false);
    fn.is_builtin = true;
    fn.param_types = (DataType *)params;
    fn.param_count = param_count;
    return symtable_insert(table, name, &fn);
}

SymTableResult symtable_init_builtins(SymTable *global_table){
    if (global_table == NULL) return SYMTABLE_INVALID_ARG;

    const DataType p_any[] = {TYPE_ANY};
    const DataType p_double[] = {TYPE_DOUBLE};
    const DataType p_string[] = {TYPE_STRING};
    const DataType p_int[] = {TYPE_INT};
    const DataType p_substr[] = {TYPE_STRING, TYPE_INT, TYPE_INT};
    const DataType p_ord[] = {TYPE_STRING, TYPE_INT};

    SymTableResult rc;

    if ((rc = register_builtin(global_table, "print", TYPE_UNIT, p_any, 1)) != SYMTABLE_OK) return rc;
    if ((rc = register_builtin(global_table, "readLine", TYPE_STRING, NULL, 0)) != SYMTABLE_OK) return rc;
    if ((rc = register_builtin(global_table, "readInt", TYPE_INT, NULL, 0)) != SYMTABLE_OK) return rc;
    if ((rc = register_builtin(global_table, "readDouble", TYPE_DOUBLE, NULL, 0)) != SYMTABLE_OK) return rc;
    if ((rc = register_builtin(global_table, "toStr", TYPE_STRING, p_any, 1)) != SYMTABLE_OK) return rc;
    if ((rc = register_builtin(global_table, "toInt", TYPE_INT, p_double, 1)) != SYMTABLE_OK) return rc;
    if ((rc = register_builtin(global_table, "strLength", TYPE_INT, p_string, 1)) != SYMTABLE_OK) return rc;
    if ((rc = register_builtin(global_table, "subStr", TYPE_STRING, p_substr, 3)) != SYMTABLE_OK) return rc;
    if ((rc = register_builtin(global_table, "ord", TYPE_INT, p_ord, 2)) != SYMTABLE_OK) return rc;
    if ((rc = register_builtin(global_table, "chr", TYPE_STRING, p_int, 1)) != SYMTABLE_OK) return rc;

    //rozšíření BOOLTHEN
    if ((rc = register_builtin(global_table, "readBoolean", TYPE_BOOL, NULL, 0)) != SYMTABLE_OK) return rc;

    return SYMTABLE_OK;
}

void symtable_stack_init(SymTableStack *stack){
    if (stack == NULL) return;
    stack->top = NULL;
    stack->next_scope_id = 0;
}

SymTableResult symtable_stack_push(SymTableStack *stack){
    if (stack == NULL) return SYMTABLE_INVALID_ARG;

    ScopeFrame *frame = malloc(sizeof(ScopeFrame));
    if (frame == NULL) return SYMTABLE_ALLOC_ERR;

    symtable_init(&frame->table);
    frame->scope_id = stack->next_scope_id++;
    frame->prev = stack->top;
    stack->top = frame;
    return SYMTABLE_OK;
}

void symtable_stack_pop(SymTableStack *stack){
    if (stack == NULL || stack->top == NULL) return;

    ScopeFrame *old_top = stack->top;
    stack->top = old_top->prev;
    symtable_dispose(&old_top->table);
    free(old_top);
}

SymTableResult symtable_stack_insert(SymTableStack *stack, const char *key, const SymbolData *data){
    if (stack == NULL || stack->top == NULL || key == NULL || data == NULL) return SYMTABLE_INVALID_ARG;

    SymbolData copy = *data;
    if(copy.symbol_type == SYMBOL_VAR){
        copy.scope_id = stack->top->scope_id;
    }

    return symtable_insert(&stack->top->table, key, &copy);
}

SymbolData *symtable_stack_search(const SymTableStack *stack, const char *key){
    if (stack == NULL ||  key == NULL) return NULL;

    for (ScopeFrame *curr = stack->top; curr != NULL; curr = curr->prev) {
        SymbolData *found = symtable_search(curr->table, key);
        if (found != NULL) return found;
    }
    return NULL;
}

SymbolData *symtable_stack_search_current(const SymTableStack *stack, const char *key){
    if (stack == NULL || stack->top == NULL || key == NULL) return NULL;

    return symtable_search(stack->top->table, key);
}

void symtable_stack_dispose(SymTableStack *stack){
    if (stack == NULL) return;

    while (stack->top != NULL) {
        symtable_stack_pop(stack);
    }
    stack->next_scope_id = 0;
}