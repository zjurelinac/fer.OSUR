#include <stdio.h>
#include <api/malloc.h>


typedef struct _node_t_ {
    struct _node_t_ *left, *right;
    int value;
} node_t;

static node_t *tree_add(node_t *node, int value){
    if(node == NULL){
        node = (node_t*) malloc(sizeof(node_t));
        node->left = NULL;
        node->right = NULL;
        node->value = value;
    } else if(value < node->value)
        node->left = tree_add(node->left, value);
    else
        node->right = tree_add(node->right, value);
    return node;
}

static node_t *tree_remove(node_t *node, int value){
    node_t *tmp;
    if(node == NULL)
        return NULL;
    else if(node->value == value){
        if(node->right == NULL)
            return node->left;
        else if(node->left == NULL)
            return node->right;
        else{
            tmp = node->right;
            while(tmp->left != NULL)
                tmp = tmp->left;
            tmp->left = node->left;
            tmp = node->right;
            free(node);
            return tmp;
        }
    } else if(value < node->value)
        node->left = tree_remove(node->left, value);
    else if(value > node->value)
        node->right = tree_remove(node->right, value);
    return node;
}

static void tree_print(node_t *node, int depth){
    int i;
    if(node == NULL) return;
    tree_print(node->left, depth + 1);
    for(i = 0; i < depth; ++i) printf(" ");
    printf("%d\n", node->value);
    tree_print(node->right, depth + 1);
}

static node_t *root;

static void bt_add(int value){
    root = tree_add(root, value);
}

static void bt_remove(int value){
    root = tree_remove(root, value);
}

static void bt_print(){
    tree_print(root, 0);
}

void bintree_test(){
    int p[] = {5, 3, 7, 6, 4, 9, 1, 17, 8}, i;

    printf("\n\nBintree_test\n");

    char *q = malloc(64 - 2 * sizeof(size_t));
    q[0] = 'a';
    printf("%s\n", q);

    return;

    for(i = 0; i < 9; ++i)
        bt_add(p[i]);

    bt_print();

    bt_remove(5);
    bt_remove(3);

    printf("\n");
    bt_print();
}
