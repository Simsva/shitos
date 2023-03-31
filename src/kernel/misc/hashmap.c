#include <kernel/hashmap.h>
#include <kernel/kmem.h>

#include <string.h>

/**
 * Create a hashmap with string keys
 */
hashmap_t *hashmap_create_str(size_t sz) {
    hashmap_t *map = kmalloc(sizeof(hashmap_t));
    map->sz = sz;
    map->entries = kmalloc(sz * sizeof(hashmap_entry_t));
    memset(map->entries, 0, sz * sizeof(hashmap_entry_t));
    map->hash = hashmap_str_hash;
    map->compar = hashmap_str_compar;
    map->key_free = map->value_free = kfree;
    map->dup = hashmap_str_dup;
    return map;
}

/**
 * Create a hashmap with integer keys
 */
hashmap_t *hashmap_create_int(size_t sz) {
    hashmap_t *map = kmalloc(sizeof(hashmap_t));
    map->sz = sz;
    map->entries = kmalloc(sz * sizeof(hashmap_entry_t));
    memset(map->entries, 0, sz * sizeof(hashmap_entry_t));
    map->hash = hashmap_int_hash;
    map->compar = hashmap_int_compar;
    map->key_free = NULL;
    map->value_free = kfree;
    map->dup = hashmap_int_dup;
    return map;
}

/**
 * Set key to val in a hashmap, return the previous value if one existed
 */
void *hashmap_set(hashmap_t *map, const void *key, void *val) {
    size_t hash;
    hashmap_entry_t *entry, *prev = NULL;

    hash = map->hash(key) % map->sz;

    for(entry = map->entries[hash]; ; prev = entry, entry = entry->next) {
        if(!entry) {
            hashmap_entry_t *e = kmalloc(sizeof(hashmap_entry_t));
            e->key = map->dup(key);
            e->value = val;
            e->next = NULL;
            if(prev) prev->next = e;
            else map->entries[hash] = e;
            return NULL;
        }
        /* key already exists */
        if(!map->compar(entry->key, key)) {
            void *tmp = entry->value;
            entry->value = val;
            return tmp;
        }
    }
}

/**
 * Get a value by key from a hashmap
 */
void *hashmap_get(hashmap_t *map, const void *key) {
    size_t hash;
    hashmap_entry_t *entry;

    hash = map->hash(key) % map->sz;
    entry = map->entries[hash];

    while(entry && map->compar(entry->key, key)) entry = entry->next;
    return entry ? entry->value : NULL;
}

/**
 * Delete an entry from a hashmap
 */
void hashmap_delete(hashmap_t *map, const void *key) {
    size_t hash;
    hashmap_entry_t *entry, *prev = NULL;

    hash = map->hash(key) % map->sz;
    entry = map->entries[hash];

    while(entry && map->compar(entry->key, key))
        prev = entry, entry = entry->next;
    if(!entry) return;
    if(prev) prev->next = entry->next;
    else map->entries[hash] = entry->next;

    if(map->key_free) map->key_free(entry->key);
    if(map->value_free) map->value_free(entry->value);
    kfree(entry);
    return;
}

/**
 * Deletes an entry from a hashmap but return the value.
 * The value needs to be freed by the user.
 */
void *hashmap_pop(hashmap_t *map, const void *key) {
    size_t hash;
    hashmap_entry_t *entry, *prev = NULL;
    void *ret;

    hash = map->hash(key) % map->sz;
    entry = map->entries[hash];

    while(entry && map->compar(entry->key, key))
        prev = entry, entry = entry->next;
    if(!entry) return NULL;
    if(prev) prev->next = entry->next;
    else map->entries[hash] = entry->next;

    ret = entry->value;
    if(map->key_free) map->key_free(entry->key);
    kfree(entry);
    return ret;
}

/**
 * Return a non-zero value if the hashmap contains the provided key
 */
int hashmap_has(hashmap_t *map, const void *key) {
    size_t hash;
    hashmap_entry_t *entry;

    hash = map->hash(key) % map->sz;
    entry = map->entries[hash];

    while(entry && map->compar(entry->key, key)) entry = entry->next;
    return entry != NULL;
}

/**
 * Return a non-zero value if the hashmap is empty
 */
int hashmap_empty(hashmap_t *map) {
    for(size_t i = 0; i < map->sz; i++)
        if(map->entries[i]) return 0;
    return 1;
}

/**
 * Return an unsorted list of all keys in a hashmap
 */
list_t *hashmap_keys(hashmap_t *map) {
    list_t *out = list_create();

    for(size_t i = 0; i < map->sz; i++)
        for(hashmap_entry_t *e = map->entries[i]; e; e = e->next)
            list_push_item(out, (list_item_t)e->key);

    return out;
}

/**
 * Return an unsorted list of all values in a hashmap
 */
list_t *hashmap_values(hashmap_t *map) {
    list_t *out = list_create();

    for(size_t i = 0; i < map->sz; i++)
        for(hashmap_entry_t *e = map->entries[i]; e; e = e->next)
            list_push_item(out, (list_item_t)e->value);

    return out;
}

/**
 * Free all hashmap entries and the hashmap itself
 */
void hashmap_free(hashmap_t *map) {
    for(size_t i = 0; i < map->sz; i++) {
        hashmap_entry_t *e = map->entries[i], *p;
        while(e) {
            p = e;
            e = e->next;

            if(map->key_free) map->key_free(p->key);
            if(map->value_free) map->value_free(p->value);
            kfree(p);
        }
    }
    kfree(map->entries);
    kfree(map);
}

/**
 * sbdm hash function
 */
size_t hashmap_str_hash(const void *key) {
    size_t hash = 0;
    char *k = (char *)key;
    int c;

    while((c = *k++))
        hash = c + (hash << 6) + (hash << 16) - hash;
    return hash;
}

int hashmap_str_compar(const void *a, const void *b) {
    return strcmp(a, b);
}

void *hashmap_str_dup(const void *key) {
    return strdup(key);
}

size_t hashmap_int_hash(const void *key) {
    return (uintptr_t)key;
}

int hashmap_int_compar(const void *a, const void *b) {
    return (intptr_t)(a - b);
}

void *hashmap_int_dup(const void *key) {
    return (void *)key;
}
