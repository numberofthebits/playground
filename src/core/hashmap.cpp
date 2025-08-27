#include "hashmap.h"

#include "log.h"

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* size_t hash(const void *ptr, size_t len) { */
/*   const char *data = ptr; */
/*   const size_t block_size = sizeof(uint32_t); */
/*   const size_t blocks_end = len - len % sizeof(uint32_t); */
/*   size_t hash = len; */
/*   size_t i = 0; */
/*   for (; i < blocks_end; i += block_size) { */
/*     size_t tmp = ((data[i] << 24) | (data[i + 1] << 16) | (data[i + 2] << 8)
 * | */
/*                   data[i + 3]); */

/*     hash += tmp; */
/*     tmp = ((data[i]) | (data[i + 1]) | (data[i + 2]) | data[i + 3]); */
/*     hash ^= tmp; */
/*   } */

/*   const size_t remainder = len % sizeof(size_t); */

/*   switch (remainder) { */
/*   case 0: */
/*   default: */
/*     break; */
/*   case 1: */
/*     hash ^= (data[i] << 24); */
/*     break; */
/*   case 2: */
/*     hash ^= ((data[i] << 24) | (data[i + 1] << 16)); */
/*     break; */
/*   case 3: */
/*     hash ^= ((data[i] << 24) | (data[i + 1] << 16) | (data[i + 2] << 8)); */
/*     break; */
/*   } */

/*   return hash; */
/* } */

static inline uint32_t murmur_32_scramble(uint32_t k) {
  k *= 0xcc9e2d51;
  k = (k << 15) | (k >> 17);
  k *= 0x1b873593;
  return k;
}

uint32_t murmur3_32(const uint8_t *key, size_t len, uint32_t seed) {
  uint32_t h = seed;
  uint32_t k;
  /* Read in groups of 4. */
  for (size_t i = len >> 2; i; i--) {
    // Here is a source of differing results across endiannesses.
    // A swap here has no effects on hash properties though.
    memcpy(&k, key, sizeof(uint32_t));
    key += sizeof(uint32_t);
    h ^= murmur_32_scramble(k);
    h = (h << 13) | (h >> 19);
    h = h * 5 + 0xe6546b64;
  }
  /* Read the rest. */
  k = 0;
  for (size_t i = len & 3; i; i--) {
    k <<= 8;
    k |= key[i - 1];
  }
  // A swap is *not* necessary here because the preceding loop already
  // places the low bytes in the low places according to whatever endianness
  // we use. Swaps only apply when the memory is copied in a chunk.
  h ^= murmur_32_scramble(k);
  /* Finalize. */
  h ^= len;
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

size_t hash(const void *ptr, size_t len) {
  return (size_t)murmur3_32((uint8_t *)ptr, len, 0);
}

void hash_map_init(HashMap *map, unsigned int size) {
  if (!map) {
    LOG_ERROR("Failed to init hash map. Nullptr");
    return;
  }

  if (!size) {
    LOG_ERROR("Invalid hash map size %d", size);
    return;
  }

  int size_bytes = sizeof(HashMapEntry) * size;
  map->entries = (HashMapEntry **)malloc(size_bytes);
  map->size = size;

  memset(map->entries, 0x0, size_bytes);
}

void hash_map_insert(HashMap *map, const void *key, size_t key_len,
                     void *value) {
  unsigned int index = hash(key, key_len) % map->size;

  HashMapEntry *entry = (HashMapEntry *)malloc(sizeof(HashMapEntry));
  entry->value = value;
  entry->next = 0;
  entry->key_len = key_len;
  entry->key = malloc(key_len);
  memcpy(entry->key, key, key_len);

  if (!map->entries[index]) {
    map->entries[index] = entry;
    return;
  }

  HashMapEntry *iter = map->entries[index];
  while (iter->next != 0) {
    iter = (HashMapEntry *)iter->next;
  }

  iter->next = entry;
}

void hash_map_insert_copy(HashMap *map, void *key, size_t key_len, void *value,
                          size_t value_len) {
  unsigned int index = hash(key, key_len) % map->size;

  HashMapEntry *entry = (HashMapEntry *)malloc(sizeof(HashMapEntry));
  entry->value = value;
  entry->next = 0;
  entry->key_len = key_len;

  entry->key = malloc(key_len);
  memcpy(entry->key, key, key_len);

  entry->value = malloc(value_len);
  memcpy(entry->value, value, value_len);

  if (!map->entries[index]) {
    map->entries[index] = entry;
    return;
  }

  HashMapEntry *iter = map->entries[index];
  while (iter->next != 0) {
    iter = (HashMapEntry *)iter->next;
  }

  iter->next = entry;
}

int hash_map_get(HashMap *map, const void *key, size_t key_len, void **value) {
  unsigned int index = hash(key, key_len) % map->size;
  HashMapEntry *entry = map->entries[index];
  if (!entry) {
    return 0;
  }

  while (entry != 0) {
    size_t min_len = key_len < entry->key_len ? key_len : entry->key_len;
    if (memcmp(key, entry->key, min_len) == 0) {
      *value = entry->value;
      return 1;
    }

    entry = (HashMapEntry *)entry->next;
  }

  return 0;
}

void *hash_map_remove(HashMap *map, void *ptr, unsigned int len) {
  void *value = 0;
  unsigned int index = hash(ptr, len) % map->size;
  HashMapEntry **head = &map->entries[index];
  if (!head) {
    LOG_WARN("Entry in hash map not found");
    return value;
  }

  HashMapEntry *iter = *head;
  HashMapEntry *previous = 0;
  for (;;) {
    if (memcmp(ptr, iter->key, len) == 0) {
      if (!previous) {
        // We removed the first head and need
        // to update head
        *head = (HashMapEntry *)iter->next;
      } else {
        // Unlink
        previous->next = iter->next;
      }
      value = iter->value;
      free(iter);
      break;
    }

    if (iter->next == 0) {
      break;
    }

    previous = iter;
    iter = (HashMapEntry *)iter->next;
  }

  return value;
}

void hash_map_print(HashMap *map) {

  for (int i = 0; i < map->size; ++i) {
    HashMapEntry *iter = map->entries[i];
    printf("%d\t--- ", i);
    if (iter == 0) {
      printf("\n");
    } else {
      int chain = 0;
      while (iter != 0) {
        printf("\n\t%d --- value %p current %p => next %p\n", chain,
               (void *)iter->value, (void *)iter, (void *)iter->next);
        chain++;
        iter = (HashMapEntry *)iter->next;
      }
    }
  }
}

void hash_map_print_func(HashMap *map, HashMapPrinterFunc printer) {
  for (int i = 0; i < map->size; ++i) {
    HashMapEntry *iter = map->entries[i];
    printf("%d\t--- ", i);
    if (iter == 0) {
      printf("\t%d --- \n", i);
    } else {
      int chain = 0;
      while (iter != 0) {
        printf("\t%d: ", chain);
        printer(iter->key, iter->key_len, iter->value);
        chain++;
        iter = (HashMapEntry *)iter->next;
      }
    }
  }
}

void hash_map_destroy(HashMap *map) {
  for (int i = 0; i < map->size; ++i) {
    HashMapEntry *entry = map->entries[i];
    while (entry != 0) {
      if (entry->value) {
        free(entry->value);
      }
      entry = (HashMapEntry *)entry->next;
      free(entry);
    }
  }

  free(map->entries);
}
