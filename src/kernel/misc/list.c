#include <kernel/list.h>

#include <string.h>
#include <kernel/kmem.h>

static inline list_node_t *list_nodeify(list_item_t item) {
    list_node_t *node = kmalloc(sizeof(list_node_t));
    memset(node, 0, sizeof(list_node_t));
    node->value = item;
    return node;
}

static inline list_node_t *list_insert_first(list_t *list, list_node_t *node) {
    list->head = list->tail = node;
    node->next = node->prev = NULL;
    list->sz++;
    return node;
}

/* create a new list */
list_t *list_create() {
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
list_node_t *list_push(list_t *list, list_node_t *node) {
    return list_insert_before(list, NULL, node);
}

/* append item at the end of list */
list_node_t *list_push_item(list_t *list, list_item_t item) {
    return list_push(list, list_nodeify(item));
}

/* insert node into list at index idx */
list_node_t *list_insert(list_t *list, size_t idx, list_node_t *node) {
    list_node_t *after = list_get(list, idx);
    if(node) return list_insert_before(list, after, node);
    return NULL;
}

/* insert item into list at index idx */
list_node_t *list_insert_item(list_t *list, size_t idx, list_item_t item) {
    return list_insert(list, idx, list_nodeify(item));
}

/* insert node into list after the specified node */
list_node_t *list_insert_after(list_t *list, list_node_t *before, list_node_t *node) {
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
    return list_insert_after(list, before, list_nodeify(item));
}

/* insert node into list before the specified node */
list_node_t *list_insert_before(list_t *list, list_node_t *after, list_node_t *node) {
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
    return list_insert_before(list, after, list_nodeify(item));
}

/* delete node at index idx from list */
list_node_t *list_remove(list_t *list, size_t idx) {
    return list_delete(list, list_get(list, idx));
}

/* delete node from list */
list_node_t *list_delete(list_t *list, list_node_t *node) {
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

/* deletes and returns the last node in the list
 * if you don't need it you should still probably free it:
 * kfree(list_pop(list)); */
list_node_t *list_pop(list_t *list) {
    if(!list->tail) return NULL;
    list_node_t *out = list->tail;
    return list_delete(list, out);
}

/* deletes and returns the first node in the list
 * if you don't need it you should still probably free it:
 * kfree(list_dequeue(list)); */
list_node_t *list_dequeue(list_t *list) {
    if(!list->head) return NULL;
    list_node_t *out = list->head;
    return list_delete(list, out);
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
