#include <kernel/list.h>

#include <features.h>
#include <string.h>
#include <stdio.h>
#include <kernel/kmem.h>

static void list_print_ptr(size_t i, list_item_t item);
static void list_print_str(size_t i, list_item_t item);

/* make a node out of an item */
static inline list_node_t *list_nodeify(list_item_t item) {
    list_node_t *node = kmalloc(sizeof(list_node_t));
    memset(node, 0, sizeof(list_node_t));
    node->value = item;
    return node;
}

/* insert a node into an empty list */
static inline list_node_t *list_insert_first(list_t *list, list_node_t *node) {
    list->head = list->tail = node;
    node->next = node->prev = NULL;
    list->sz++;
    return node;
}

/* normal list_compar_t function */
static int list_compar_normal(list_item_t a, list_item_t b) {
    return (intptr_t)a - (intptr_t)b;
}

/* create a new list */
list_t *list_create(void) {
    list_t *out = kmalloc(sizeof(list_t));
    return memset(out, 0, sizeof(list_t));
}

/* free all node contents */
void list_destroy(list_t *list) {
    list_node_t *n = list->head;
    while(n) {
        kfree(n->value);
        n = n->next;
    }
}

/* free list_t and list_node_t structures but not node contents */
void list_free(list_t *list) {
    list_node_t *n = list->head;
    while(n) {
        list_node_t *tmp = n->next;
        kfree(n);
        n = tmp;
    }
    kfree(list);
}

/* append node at the end of list */
list_node_t *list_push_node(list_t *list, list_node_t *node) {
    return list_insert_node_before(list, NULL, node);
}
weak_alias(list_push_node, list_enqueue_node);

/* append item at the end of list */
list_node_t *list_push_item(list_t *list, list_item_t item) {
    return list_push_node(list, list_nodeify(item));
}
weak_alias(list_push_item, list_enqueue_item);
weak_alias(list_push_item, list_push);
weak_alias(list_push_item, list_enqueue);

/* insert node into list at index idx */
list_node_t *list_insert_node(list_t *list, size_t idx, list_node_t *node) {
    list_node_t *after = list_get(list, idx);
    if(node) return list_insert_node_before(list, after, node);
    return NULL;
}

/* insert item into list at index idx */
list_node_t *list_insert_item(list_t *list, size_t idx, list_item_t item) {
    return list_insert_node(list, idx, list_nodeify(item));
}
weak_alias(list_insert_item, list_insert);

/* insert node into list after the specified node */
list_node_t *list_insert_node_after(list_t *list, list_node_t *before, list_node_t *node) {
    node->owner = list;
    if(!list->sz) return list_insert_first(list, node);
    if(before == NULL) {
        /* interpret NULL as after the NULL at the beginning
         * i.e. prepend to list */
        node->next = list->head;
        node->prev = NULL;
        list->head->prev = node;
        list->head = node;
        list->sz++;
        return node;
    }
    if(before == list->tail) {
        list->tail = node;
    } else {
        before->next->prev = node;
        node->next = before->next;
    }
    node->prev = before;
    before->next = node;
    list->sz++;
    return node;
}

/* insert item into list after the specified node */
list_node_t *list_insert_item_after(list_t *list, list_node_t *before, list_item_t item) {
    return list_insert_node_after(list, before, list_nodeify(item));
}
weak_alias(list_insert_item_after, list_insert_after);

/* insert node into list before the specified node */
list_node_t *list_insert_node_before(list_t *list, list_node_t *after, list_node_t *node) {
    node->owner = list;
    if(!list->sz) return list_insert_first(list, node);
    if(after == NULL) {
        /* interpret NULL as before the NULL at the end
         * i.e. append to list */
        node->next = NULL;
        node->prev = list->tail;
        list->tail->next = node;
        list->tail = node;
        list->sz++;
        return node;
    }
    if(after == list->head) {
        list->tail = node;
    } else {
        after->prev->next = node;
        node->prev = after->prev;
    }
    node->next = after;
    after->prev = node;
    list->sz++;
    return node;
}

/* insert item into list before the specified node */
list_node_t *list_insert_item_before(list_t *list, list_node_t *after, list_item_t item) {
    return list_insert_node_before(list, after, list_nodeify(item));
}
weak_alias(list_insert_item_before, list_insert_before);

/* delete node from list */
list_node_t *list_delete_node(list_t *list, list_node_t *node) {
    if(node == list->head)
        list->head = node->next;
    if(node == list->tail)
        list->tail = node->prev;
    if(node->prev)
        node->prev->next = node->next;
    if(node->next)
        node->next->prev = node->prev;
    node->next = node->prev = NULL;
    node->owner = NULL;
    list->sz--;
    return node;
}
weak_alias(list_delete_node, list_delete);

/* delete node at index idx from list */
list_node_t *list_delete_idx(list_t *list, size_t idx) {
    return list_delete_node(list, list_get(list, idx));
}

/* deletes and returns the last node in the list
 * if you don't need it you should still probably free it:
 * kfree(list_pop(list)); */
list_node_t *list_pop(list_t *list) {
    if(!list->tail) return NULL;
    list_node_t *out = list->tail;
    return list_delete_node(list, out);
}

/* deletes and returns the first node in the list
 * if you don't need it you should still probably free it:
 * kfree(list_dequeue(list)); */
list_node_t *list_dequeue(list_t *list) {
    if(!list->head) return NULL;
    list_node_t *out = list->head;
    return list_delete_node(list, out);
}

/* returns the node at index idx in list (or NULL) */
list_node_t *list_get(list_t *list, size_t idx) {
    size_t i = 0;
    list_foreach(item, list)
        if(i++ == idx) return item;
    return NULL;
}

/* creates a new copy of src */
list_t *list_copy(list_t *src) {
    list_t *out = list_create();
    list_node_t *node = src->head;
    while(node)
        list_push_item(out, node->value), node = node->next;
    return out;
}

/* destructively merges src into dst and returns dst */
list_t *list_merge(list_t *dst, list_t *src) {
    list_foreach(node, src)
        node->owner = dst;
    if(src->head)
        src->head->prev = dst->tail;
    if(dst->tail)
        dst->tail->next = src->head;
    else
        dst->head = src->head;
    if(src->tail)
        dst->tail = src->tail;
    dst->sz += src->sz;
    kfree(src);
    return dst;
}

/* return the node containing the specified item */
list_node_t *list_find(list_t *list, list_item_t item, list_compar_t compar) {
    list_foreach(node, list)
        if(!compar(node->value, item))
            return node;
    return NULL;
}

/* list_from with the default comparison function */
list_node_t *list_find_eq(list_t *list, list_item_t item) {
    return list_find(list, item, list_compar_normal);
}

/* get the index of the specified item */
ssize_t list_index_of(list_t *list, list_item_t item, list_compar_t compar) {
    ssize_t i = 0;
    list_foreach(node, list)
        if(!compar(node->value, item)) return i;
        else i++;
    return -1;
}

/* list_index_of with the default comparison function */
ssize_t list_index_of_eq(list_t *list, list_item_t item) {
    return list_index_of(list, item, list_compar_normal);
}

/* list_debug_dump callback to print pointers */
static void list_print_ptr(size_t i, list_item_t item) {
    printf("item %zu: %p\n", i, item);
}

/* list_debug_dump callback to print strings */
static void list_print_str(size_t i, list_item_t item) {
    printf("item %zu: %s\n", i, (char *)item);
}

/* print the contents of a list */
void list_debug_dump(list_t *list, void (*print)(size_t, list_item_t)) {
    if(!list || !print) return;
    size_t i = 0;
    list_foreach(node, list)
        print(i++, node->value);
}

/* list_debug_dump printing as pointers */
void list_debug_dump_ptr(list_t *list) {
    list_debug_dump(list, list_print_ptr);
}

/* list_debug_dump printing as strings */
void list_debug_dump_str(list_t *list) {
    list_debug_dump(list, list_print_str);
}
