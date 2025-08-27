#include "util.h"

#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t file_read_all_buffer_binary(const char *file_path, unsigned char *buffer,
                                   size_t buffer_size) {
  FILE *fp = fopen(file_path, "r+b");
  if (!fp) {
    return 0;
  }

  fseek(fp, 0, SEEK_END);
  int file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  LOG_INFO("File size %d bytes", file_size);
  if (file_size > (int)buffer_size) {
    LOG_ERROR("File size %zu does not fit in buffer size %d", file_size,
              buffer_size);
    return 0;
  }

  int bytes_read = fread(buffer, 1, file_size, fp);
  fclose(fp);

  return (size_t)bytes_read;
}

size_t file_read_all_buffer_text(const char *file_path, Buffer *buffer) {
  FILE *fp = fopen(file_path, "r");
  if (!fp) {
    return 0;
  }

  fseek(fp, 0, SEEK_END);
  int file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  LOG_INFO("File size %d bytes", file_size);
  if ((size_t)file_size >= buffer->capacity) {
    LOG_ERROR("File size %d does not fit in buffer size %zu", file_size,
              buffer->capacity);
    return 0;
  }

  int bytes_read = fread(buffer->data, 1, file_size, fp);
  fclose(fp);
  buffer->data[bytes_read] = 0;
  buffer->used = (size_t)bytes_read;

  return (size_t)bytes_read;
}

static inline int file_get_size_impl(FILE *fp) {
  fseek(fp, 0, SEEK_END);
  int file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  return file_size;
}

int file_read_all(const char *file_path, Buffer *buffer) {
  FILE *fp = fopen(file_path, "r+b");
  if (!fp) {
    return 0;
  }

  int file_size = file_get_size_impl(fp);

  LOG_INFO("File size %d bytes", file_size);
  buffer->data = (uint8_t *)malloc(file_size + 1);
  buffer->capacity = file_size >= 0 ? (size_t)file_size : 0;
  buffer->data[file_size] = 0;

  fread(buffer->data, file_size, 1, fp);
  fclose(fp);

  return 1;
}

size_t file_write_all_binary(const char *file_path, unsigned char *buffer,
                             size_t buffer_size) {
  FILE *fp = fopen(file_path, "w+b");
  int ret = fwrite(buffer, buffer_size, 1, fp);
  if (ret < 0) {
    return 0;
  }
  return (size_t)ret;
}

int file_get_size(const char *file_path) {
  FILE *fp = fopen(file_path, "r+b");

  if (!fp) {
    return -1;
  }

  int ret = file_get_size_impl(fp);
  fclose(fp);

  return ret;
}

int get_msb_set(uint64_t value) {
  for (size_t i = 0; i < 64; ++i) {
    if ((value >> i) & 0x1) {
      return (int)i;
    }
  }
  return -1;
}

size_t offset_to_character(const char *str, size_t len, char c) {
  const char *ptr = str;
  size_t offset = 0;

  while (offset < len && *(ptr + offset) != c) {
    ++offset;
  }

  return offset;
}
