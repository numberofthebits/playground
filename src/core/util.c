#include <core/util.h>

#include <core/log.h>

#include <stdio.h>
#include <stdlib.h>

unsigned char *file_read_all(const char *file_path) {
  FILE *fp = fopen(file_path, "r+b");
  if (!fp) {
    return 0;
  }

  fseek(fp, 0, SEEK_END);
  int size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  LOG_INFO("File size %d bytes", size);
  unsigned char *data = malloc(size + 1);
  data[size] = 0;

  fread(data, size, 1, fp);
  fclose(fp);

  return data;
}

int get_msb_set(uint64_t value) {
  for (size_t i = 0; i < 64; ++i) {
    if ((value >> i) & 0x1) {
      return (int)i;
    }
  }
  return -1;
}
