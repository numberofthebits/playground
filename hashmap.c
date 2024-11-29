#include "hashmap.h"

#include "log.h"

#include <stdlib.h>
#include <memory.h>
#include <stdio.h>

unsigned int hash(void* ptr, size_t len) {
    char* data = ptr;
    const size_t block_size = sizeof(int);
    const size_t blocks_end = len - len % sizeof(unsigned int);
    unsigned int hash = (unsigned int)len;
    size_t i = 0;
    for (; i < blocks_end; i += block_size) {
        unsigned int tmp = ((data[i] << 24) | (data[i+1] << 16) | (data[i+2] << 8) | data[i+3]);
 
        hash += tmp;
        tmp = ((data[i]) | (data[i+1] ) | (data[i+2] ) | data[i+3]);
        hash ^= tmp;
    }

    const size_t remainder = len % sizeof(unsigned);

    switch (remainder) {
    case 0:
    default:
        break;
    case 1:
        hash ^= (data[i] << 24);
        break;
    case 2:
        hash ^= ((data[i] << 24) | (data[i+1] << 16));
        break;
    case 3:
        hash ^= ((data[i] << 24) | (data[i+1] << 16) | (data[i+2] << 8));
        break;
    }
    
    return hash;
}

void hash_map_init(HashMap* map, unsigned int size) {
    if(!map) {
        LOG_ERROR("Failed to init hash map. Nullptr");
        return;
    }

    if(!size) {
        LOG_ERROR("Invalid hash map size %d", size);
        return;
    }

    int size_bytes = sizeof(HashMapEntry) * size;
    map->entries = malloc(size_bytes);
    map->size = size;
    
    memset(map->entries, 0x0, size_bytes);
}

void hash_map_insert(HashMap* map, void* key, size_t key_len, void* value) {
    unsigned int index = hash(key, key_len) % map->size;

    HashMapEntry* entry = malloc(sizeof(HashMapEntry));
    entry->value = value;
    entry->next = 0;
    entry->key_len = key_len;
    entry->key = malloc(key_len);
    memcpy(entry->key, key, key_len);

    if(!map->entries[index]) {        
        map->entries[index] = entry;
        return;
    }

    HashMapEntry* iter = map->entries[index];
    while (iter->next != 0) {
        iter = iter->next;
    }

    iter->next = entry;
}

void hash_map_insert_value(HashMap* map, void* key, size_t key_len, void* value)
{
    unsigned int index = hash(key, key_len) % map->size;

    HashMapEntry* entry = malloc(sizeof(HashMapEntry));
    entry->value = value;
    entry->next = 0;
    entry->key_len = key_len;
    entry->key = malloc(key_len);
    memcpy(entry->key, key, key_len);

    if(!map->entries[index]) {        
        map->entries[index] = entry;
        return;
    }

    HashMapEntry* iter = map->entries[index];
    while (iter->next != 0) {
        iter = iter->next;
    }

    iter->next = entry;
}


int hash_map_get(HashMap* map, void* key, size_t key_len, void** value) {
    unsigned int index = hash(key, key_len) % map->size;
    HashMapEntry* entry = map->entries[index];
    if(!entry) {
        return 0;
    }

    while(entry != 0) {
        size_t min_len = key_len < entry->key_len ? key_len : entry->key_len;
        if (memcmp(key, entry->key, min_len) == 0) {
            *value = entry->value;
            return 1;
        }

        entry = entry->next;
    }

    return 0;
}

void hash_map_delete(HashMap* map, void* ptr, unsigned int len) {
    unsigned int index = hash(ptr, len) % map->size;
    HashMapEntry* entry = map->entries[index];
    if (!entry) {
        LOG_WARN("Entry in hash map not found");
        return;
    }

    HashMapEntry* iter = entry;
    HashMapEntry* previous = 0;
    for(;;) {

        if( memcmp(ptr, iter->value, len) == 0 ) {
            if (previous) {
                // Unlink
                previous->next = iter->next;

            }
            free(iter->value);
            free(iter);
            break;
        }
        
        if (iter->next == 0) {
            break;
        }

        previous = iter;
        iter = iter->next;
    }
}

void hash_map_print(HashMap* map) {

    for(int i = 0; i < map->size; ++i) {
        HashMapEntry* iter = map->entries[i];
        printf("%d\t--- ", i);
        if(iter == 0) {
            printf("\n");
        } else {
            int chain = 0;
            while (iter != 0) {
                printf("\n\t%d --- value %p current %p => next %p\n", chain, iter->value, iter, iter->next);
                chain++;
                iter = iter->next;
            }
        }
    }
}

void hash_map_print_func(HashMap* map, HashMapPrinterFunc printer) {
    for(int i = 0; i < map->size; ++i) {
        HashMapEntry* iter = map->entries[i];
        printf("%d\t--- ", i);
        if(iter == 0) {
            printf("\t%d --- \n", i);
        } else {
            int chain = 0;
            while (iter != 0) {
                printf("\t%d: ", chain);
                printer(iter->key, iter->key_len, iter->value);
                chain++;
                iter = iter->next;
            }
        }
    }
}

void hash_map_destroy(HashMap* map) {
    for (int i = 0; i < map->size; ++i) {
        HashMapEntry* entry = map->entries[i];
        while (entry != 0) {
            if (entry->value) {
                free(entry->value);
            }
            entry = entry->next;
            free(entry);
        }
    }

    free(map->entries);
}
