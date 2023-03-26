#ifndef KERNEL_LIST_H_
#define KERNEL_LIST_H_

/* general purpose doubly-linked list */
/* partially from https://github.com/klange/toaruos */

#include <sys/types.h>
#include <stddef.h>

typedef void * list_item_t;

typedef struct list_node {
    struct list_node *next, *prev;
    list_item_t value;
    struct list *owner;
} list_node_t;

typedef struct list {
    list_node_t *head, *tail;
    size_t sz;
} list_t;

list_t *list_create(void);
void list_destroy(list_t *list);
void list_free(list_t *list);

list_node_t *list_push(list_t *list, list_node_t *node);
list_node_t *list_push_item(list_t *list, list_item_t item);
list_node_t *list_enqueue(list_t *list, list_node_t *node);
list_node_t *list_enqueue_item(list_t *list, list_item_t item);
list_node_t *list_insert(list_t *list, size_t idx, list_node_t *node);
list_node_t *list_insert_item(list_t *list, size_t idx, list_item_t item);

list_node_t *list_insert_after(list_t *list, list_node_t *before, list_node_t *node);
list_node_t *list_insert_item_after(list_t *list, list_node_t *before, list_item_t item);
list_node_t *list_insert_before(list_t *list, list_node_t *after, list_node_t *node);
list_node_t *list_insert_item_before(list_t *list, list_node_t *after, list_item_t item);

list_node_t *list_delete(list_t *list, list_node_t *node);
list_node_t *list_delete_idx(list_t *list, size_t idx);
list_node_t *list_pop(list_t *list);
list_node_t *list_dequeue(list_t *list);
list_node_t *list_get(list_t *list, size_t idx);

list_t *list_copy(list_t *src);
list_t *list_merge(list_t *dst, list_t *src);

list_node_t *list_find(list_t *list, list_item_t item);
ssize_t list_index_of(list_t *list, list_item_t item);

#define list_foreach(node, list)  for(list_node_t *node = (list)->head; node; node = node->next)
#define list_foreachr(node, list) for(list_node_t *node = (list)->tail; node; node = node->prev)

#endif // KERNEL_LIST_H_
