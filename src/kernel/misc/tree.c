#include <kernel/tree.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <kernel/kmem.h>

static void tree_debug_dump_recur(tree_node_t *node, void (*print)(tree_item_t), size_t depth);
static void tree_print_ptr(tree_item_t item);
static void tree_print_str(tree_item_t item);

/* normal tree_compar_t function */
static int tree_compar_normal(tree_item_t a, tree_item_t b) {
    return (intptr_t)a - (intptr_t)b;
}

/* create a new tree */
tree_t *tree_create(void) {
    tree_t *out = kmalloc(sizeof(tree_t));
    return memset(out, 0, sizeof(tree_t));
}

/* free the contents of a tree, but not the nodes */
void tree_destroy(tree_t *tree) {
    if(tree->root)
        tree_node_destroy(tree->root);
}

/* free all of the nodes in a tree (and itself), but not their contents */
void tree_free(tree_t *tree) {
    tree_node_free(tree->root);
    kfree(tree);
}

/* set the root node of a new tree */
void tree_set_root(tree_t *tree, tree_item_t item) {
    assert(!tree->root && "Reassigning root of an existing tree");
    tree_node_t *root = tree_node_create(item);
    tree->root = root;
    tree->sz = 1;
}

/* insert node as a child of parent (returns node) */
tree_node_t *tree_insert(tree_t *tree, tree_node_t *parent, tree_node_t *node) {
    list_push_item(parent->children, node);
    node->parent = parent;
    tree->sz++;
    return node;
}

/* insert item as a child of parent (returns the created node) */
tree_node_t *tree_insert_item(tree_t *tree, tree_node_t *parent, tree_item_t item) {
    return tree_insert(tree, parent, tree_node_create(item));
}

/* remove node and move its children into its parent's list of children
 * returns new parent */
tree_node_t *tree_remove(tree_t *tree, tree_node_t *node) {
    tree_node_t *parent = node->parent;

    /* cannot move children into a nonexistant node */
    if(!parent) return NULL;

    tree->sz--;
    list_delete(parent->children, list_find_eq(parent->children, node));
    /* reassign parents */
    list_foreach(child, node->children)
        ((tree_node_t *)child->value)->parent = parent;
    list_merge(parent->children, node->children);
    kfree(node);
    return parent;
}

/* removes a branch from tree given the root node */
void tree_remove_branch(tree_t *tree, tree_node_t *node) {
    tree_node_t *parent = node->parent;

    if(!parent) {
        if(node == tree->root) {
            tree->sz = 0;
            tree->root = NULL;
            tree_node_free(node);
        }
        return;
    }
    tree_remove_parent(tree, parent, node);
}

/* removes a branch from tree given the root node and its parent */
void tree_remove_parent(tree_t *tree, tree_node_t *parent, tree_node_t *node) {
    tree->sz -= tree_node_count_children(node) + 1;
    list_delete(parent->children, list_find_eq(parent->children, node));
    tree_node_free(node);
}

/* find node containing item in tree */
tree_node_t *tree_find(tree_t *tree, tree_item_t item, tree_compar_t compar) {
    return tree_node_find(tree->root, item, compar);
}

/* tree_find with the default comparison function */
tree_node_t *tree_find_eq(tree_t *tree, tree_item_t item) {
    return tree_find(tree, item, tree_compar_normal);
}

/* create a new tree node containing item */
tree_node_t *tree_node_create(tree_item_t item) {
    tree_node_t *out = kmalloc(sizeof(tree_node_t));
    out->value = item;
    out->children = list_create();
    out->parent = NULL;
    return out;
}

/* free the contents of a node and its children, but not the nodes themselves */
void tree_node_destroy(tree_node_t *node) {
    if(!node) return;
    list_foreach(child, node->children)
        tree_node_destroy((tree_node_t *)child->value);
    kfree(node->value);
}

/* free a node and its children, but not their contents */
void tree_node_free(tree_node_t *node) {
    if(!node) return;
    list_foreach(child, node->children)
        tree_node_free((tree_node_t *)child->value);
    list_free(node->children);
    kfree(node);
}

/* recursively search the children of node for item */
tree_node_t *tree_node_find(tree_node_t *node, tree_item_t item, tree_compar_t compar) {
    if(compar(node->value, item) == 0) return node;

    tree_node_t *found;
    list_foreach(child, node->children)
        if((found = tree_node_find((tree_node_t *)child->value, item, compar)))
            return found;
    return NULL;
}

/* tree_node_find with the default comparison function */
tree_node_t *tree_node_find_eq(tree_node_t *node, tree_item_t item) {
    return tree_node_find(node, item, tree_compar_normal);
}

/* return the number of (recursive) children this node has */
size_t tree_node_count_children(tree_node_t *node) {
    if(!node || !node->children) return 0;
    size_t out = node->children->sz;
    list_foreach(child, node->children)
        out += tree_node_count_children((tree_node_t *)child->value);
    return out;
}

static void tree_debug_dump_recur(tree_node_t *node, void (*print)(tree_item_t), size_t depth) {
    for(size_t i = 0; i < depth; i++) putchar(' ');
    print(node->value);

    list_foreach(child, node->children)
        tree_debug_dump_recur((tree_node_t *)child->value, print, depth + 1);
}

/* tree_debug_dump callback to print pointers */
static void tree_print_ptr(tree_item_t item) {
    printf("%p\n", item);
}

/* tree_debug_dump callback to print strings */
static void tree_print_str(tree_item_t item) {
    printf("%s\n", (char *)item);
}

/* print the contents of a tree */
void tree_debug_dump(tree_t *tree, void (*print)(tree_item_t)) {
    if(!tree || !print) return;
    tree_debug_dump_recur(tree->root, print, 0);
}

/* tree_debug_dump printing as pointers */
void tree_debug_dump_ptr(tree_t *tree) {
    tree_debug_dump(tree, tree_print_ptr);
}

/* tree_debug_dump printing as strings */
void tree_debug_dump_str(tree_t *tree) {
    tree_debug_dump(tree, tree_print_str);
}
