#ifndef KERNEL_HASHMAP_H_
#define KERNEL_HASHMAP_H_

#include <stddef.h>
#include <kernel/list.h>

typedef size_t (*hashmap_hash_t)(const void *);
typedef int (*hashmap_compar_t)(const void *, const void *);
typedef void (*hashmap_free_t)(void *);
typedef void *(*hashmap_dup_t)(const void *);

typedef struct hashmap_entry {
    void *key, *value;
    struct hashmap_entry *next;
} hashmap_entry_t;

typedef struct hashmap {
    hashmap_hash_t hash;
    hashmap_compar_t compar;
    hashmap_free_t key_free, value_free;
    hashmap_dup_t dup;
    size_t sz;
    hashmap_entry_t **entries;
} hashmap_t;

hashmap_t *hashmap_create_str(size_t sz);
hashmap_t *hashmap_create_int(size_t sz);
void *hashmap_set(hashmap_t *map, const void *key, void *val);
void *hashmap_get(hashmap_t *map, const void *key);
void hashmap_delete(hashmap_t *map, const void *key);
void *hashmap_pop(hashmap_t *map, const void *key);
int hashmap_has(hashmap_t *map, const void *key);
int hashmap_empty(hashmap_t *map);
list_t *hashmap_keys(hashmap_t *map);
list_t *hashmap_values(hashmap_t *map);
void hashmap_free(hashmap_t *map);

size_t hashmap_str_hash(const void *key);
int hashmap_str_compar(const void *a, const void *b);
void *hashmap_str_dup(const void *key);
size_t hashmap_int_hash(const void *key);
int hashmap_int_compar(const void *a, const void *b);
void *hashmap_int_dup(const void *key);

#endif // KERNEL_HASHMAP_H_
