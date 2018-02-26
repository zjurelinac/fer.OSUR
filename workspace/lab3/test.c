#include <stdio.h>
#include <string.h>
#include "vbs.h"

#define MAX_SIZE (2*1024*1024)

char heap[MAX_SIZE];
char *p, *q, *r;


typedef struct _node_t_ {
    struct _node_t_ *left, *right;
    int value;
} node_t;

// Sorted binary tree implementation

static node_t *tree_add(node_t *node, int value){
    if(node == NULL){
        node = (node_t*) m_alloc(sizeof(node_t));
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
            m_free(node);
            return tmp;
        }
    } else if(value < node->value)
        node->left = tree_remove(node->left, value);
    else if(value > node->value)
        node->right = tree_remove(node->right, value);
}

static void *tree_print(node_t *node, int depth){
    int i;
    if(node == NULL) return;
    tree_print(node->left, depth + 1);
    for(i = 0; i < depth; ++i) putchar(' ');
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

int main(){
    printf("Testing VBS memory allocation, START = %x, END = %x, SIZE = %lu\n", heap, heap+MAX_SIZE, MAX_SIZE);
    m_init((void*) &heap, MAX_SIZE);
    bt_add(5);
    bt_add(3);
    bt_add(7);
    bt_add(6);
    bt_add(4);
    bt_add(9);
    bt_add(1);
    bt_add(17);
    bt_add(8);
    bt_print();

    bt_remove(5);
    bt_remove(3);
    puts("\n");
    bt_print();
}
