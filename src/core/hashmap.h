#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdlib.h>

typedef struct {
  void *next;
  void *key;
  size_t key_len;
  void *value;
} HashMapEntry;

typedef struct {
  HashMapEntry **entries;
  int size;
} HashMap;

// Homemade non-cryptographic hash function. This should not
// be used for anyone for anything except educational purposes,
// perhaps as an example for why not to try and write your own
// hash functions.
unsigned int hash(const void *ptr, size_t len);

// Size is unique entries size
void hash_map_init(HashMap *map, unsigned int size);

void hash_map_insert(HashMap *map, void *key, size_t key_len, void *value_ptr);

// Abuse the pointer storage to store an actual value, avoiding a pointer chase
void hash_map_insert_value(HashMap *map, void *key, size_t key_len,
                           void *value);

int hash_map_get(HashMap *map, void *key, size_t key_len, void **value);

void hash_map_print(HashMap *map);

typedef void (*HashMapPrinterFunc)(void *key, size_t key_len, void *value);
void hash_map_print_func(HashMap *map, HashMapPrinterFunc printer);

void hash_map_destroy(HashMap *map);

#endif
