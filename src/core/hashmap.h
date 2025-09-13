#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdlib.h>

struct HashMapEntry {
  HashMapEntry *next;
  void *key;
  size_t key_len;
  void *value;
};

typedef struct {
  HashMapEntry **entries;
  int size;
} HashMap;

// Homemade non-cryptographic hash function. This should not
// be used for anyone for anything except educational purposes,
// perhaps as an example for why not to try and write your own
// hash functions.
size_t hash(const void *ptr, size_t len);

// Size is unique entries size
void hash_map_init(HashMap *map, unsigned int size);

void hash_map_insert(HashMap *map, const void *key, size_t key_len,
                     void *value_ptr);

int hash_map_get(HashMap *map, const void *key, size_t key_len, void **value);

void *hash_map_remove(HashMap *map, void *ptr, unsigned int len);

void hash_map_print(HashMap *map);

typedef void (*HashMapPrinterFunc)(void *key, size_t key_len, void *value);
void hash_map_print_func(HashMap *map, HashMapPrinterFunc printer);

void hash_map_destroy(HashMap *map);

void hash_map_log(HashMap *map);

#endif
