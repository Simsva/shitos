#ifndef KERNEL_LIST_H_
#define KERNEL_LIST_H_

/* general purpose doubly-linked list */
/* partially from https://github.com/klange/toaruos */

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

list_t *list_create();
void list_destroy(list_t *list);
void list_free(list_t *list);

list_node_t *list_push(list_t *list, list_node_t *node);
list_node_t *list_push_item(list_t *list, list_item_t item);
/* alias enqueue to push */
#define list_enqueue(list, node) list_push(list, node)
#define list_enqueue_item(list, item) list_push_item(list, item);
list_node_t *list_insert(list_t *list, size_t idx, list_node_t *node);
list_node_t *list_insert_item(list_t *list, size_t idx, list_item_t item);

list_node_t *list_insert_after(list_t *list, list_node_t *before, list_node_t *node);
list_node_t *list_insert_item_after(list_t *list, list_node_t *before, list_item_t item);
list_node_t *list_insert_before(list_t *list, list_node_t *after, list_node_t *node);
list_node_t *list_insert_item_before(list_t *list, list_node_t *after, list_item_t item);

list_node_t *list_remove(list_t *list, size_t idx);
list_node_t *list_delete(list_t *list, list_node_t *node);
list_node_t *list_pop(list_t *list);
list_node_t *list_dequeue(list_t *list);
list_node_t *list_get(list_t *list, size_t idx);

list_t *list_copy(list_t *src);
list_t *list_merge(list_t *dst, list_t *src);

#define list_foreach(i, list)  for(list_node_t *i = (list)->head; i; i = i->next)
#define list_foreachr(i, list) for(list_node_t *i = (list)->tail; i; i = i->prev)

#endif // KERNEL_LIST_H_
