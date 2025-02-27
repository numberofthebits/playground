#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

#define Min(a, b) (((a) <= (b)) ? (a) : (b))
#define Max(a, b) (((a) > (b)) ? (a) : (b))

unsigned char *file_read_all(const char *file_path);

int get_msb_set(uint64_t value);

#endif // UTIL_H
