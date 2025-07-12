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

typedef struct Buffer {
  uint8_t *data;
  size_t len;
} Buffer;

// Mallocs for you
int file_read_all(const char *file_path, Buffer *buffer);

// You allocate for it. Fails if file size is bigger than buffer
size_t file_read_all_buffer_binary(const char *file_path, unsigned char *buffer,
                                   size_t buffer_size);

// You allocate for it. Fails if file size is bigger than buffer
size_t file_read_all_buffer_text(const char *file_path, Buffer *buffer);

int get_msb_set(uint64_t value);

size_t offset_to_character(const char *str, size_t len, char s);

#endif // UTIL_H
