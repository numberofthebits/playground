#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stdio.h>

#define Min(a, b) (((a) <= (b)) ? (a) : (b))
#define Max(a, b) (((a) > (b)) ? (a) : (b))

// TODO: __debug_brk and equivalents for non-MSVC
#define Assert(expr)                                                           \
  if (!(expr)) {                                                               \
    printf("Assertion failed %s:%d", __FILE__, __LINE__);                      \
    exit(-1);                                                                  \
  }

unsigned char *file_read_all(const char *file_path);

int get_msb_set(uint64_t value);

#endif // UTIL_H
