#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stdio.h>

#define Min(a, b) (((a) <= (b)) ? (a) : (b))
#define Max(a, b) (((a) > (b)) ? (a) : (b))

#ifdef _WIN32
#define Assert(expr)                                                           \
  if (!(expr)) {                                                               \
    printf("Assertion failed %s:%d", __FILE__, __LINE__);                      \
    __debugbreak();                                                            \
  }

#elif defined(__linux__)

#include <signal.h>
#define Assert(expr)                                                           \
  if (!(expr)) {                                                               \
    printf("Assertion failed %s:%d", __FILE__, __LINE__);                      \
    raise(SIGTRAP);
}
#endif

unsigned char *file_read_all(const char *file_path);

int get_msb_set(uint64_t value);

#endif // UTIL_H
