#ifndef KERNEL_TREE_H_
#define KERNEL_TREE_H_

/* general-purpose tree */
/* partially from https://github.com/klange/toaruos */

#include <kernel/list.h>

typedef void * tree_item_t;

typedef struct tree_node {
    tree_item_t value;
    list_t *children;
    struct tree_node *parent;
} tree_node_t;

typedef struct tree {
    size_t sz;
    tree_node_t *root;
} tree_t;

typedef int (*tree_compar_t)(tree_item_t, tree_item_t);

tree_t *tree_create(void);
void tree_destroy(tree_t *tree);
void tree_free(tree_t *tree);

void tree_set_root(tree_t *tree, tree_item_t item);
tree_node_t *tree_insert(tree_t *tree, tree_node_t *parent, tree_node_t *node);
tree_node_t *tree_insert_item(tree_t *tree, tree_node_t *parent, tree_item_t item);

tree_node_t *tree_remove(tree_t *tree, tree_node_t *node);
void tree_remove_branch(tree_t *tree, tree_node_t *node);
void tree_remove_parent(tree_t *tree, tree_node_t *parent, tree_node_t *node);

tree_node_t *tree_find(tree_t *tree, tree_item_t item, tree_compar_t compar);
tree_node_t *tree_find_eq(tree_t *tree, tree_item_t item);

tree_node_t *tree_node_create(tree_item_t item);
void tree_node_destroy(tree_node_t *node);
void tree_node_free(tree_node_t *node);

tree_node_t *tree_node_find(tree_node_t *node, tree_item_t item, tree_compar_t compar);
tree_node_t *tree_node_find_eq(tree_node_t *node, tree_item_t item);
size_t tree_node_count_children(tree_node_t *node);

void tree_debug_dump(tree_t *tree, void (*print)(tree_item_t));
void tree_debug_dump_ptr(tree_t *tree);
void tree_debug_dump_str(tree_t *tree);

#endif // KERNEL_TREE_H_
