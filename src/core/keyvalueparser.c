#include "keyvalueparser.h"

#include <string.h>

static int offset_to_char(const char *str, size_t len, char c, size_t *out) {
  const char *ptr = str;

  while (ptr < str + len) {
    if (*ptr != c) {
      ++ptr;
    } else {
      *out = ptr - str;
      return 1;
    }
  }

  return 0;
}

int get_next_key_value_raw(ParseState *state, KeyValueRaw *raw) {

  const char *begin = (const char *)state->buffer->data + state->offset;
  *raw = (KeyValueRaw){0};
  size_t line_len = 0;
  if (!offset_to_char(begin, state->buffer->used,
                      PARSER_DELIMITER_KEY_VALUE_PAIR, &line_len)) {
    LOG_WARN("Failed to find line length");
    return PARSER_ERROR;
  }

  if (!line_len) {
    // Extraneous newline should be accepted?
    return PARSER_DONE;
  }

  size_t key_len = 0;
  if (!offset_to_char(begin, line_len, PARSER_DELIMITER_KEY_VALUE, &key_len)) {
    LOG_WARN("Failed to find key length");
    return PARSER_ERROR;
  }

  size_t value_len = line_len - key_len - 1;

  raw->key_begin = begin;
  raw->key_len = key_len;
  raw->value_begin = begin + key_len + 1;
  raw->value_len = value_len;

  state->offset += line_len + 1; // skip newline

  return PARSER_OK;
}

#ifdef BUILD_TESTS
// Values out must be able to hold 'expected_count'
int parse_array_int(const char *string_val, size_t string_len,
                    size_t expected_count, int *values_out) {
  size_t offset_begin = 0;
  size_t offset_end = 0;
  size_t count = 0;

  while (count < expected_count && offset_begin < string_len) {
    if (!offset_to_char(string_val + offset_begin, string_len - offset_begin,
                        PARSER_DELIMITER_VALUE, &offset_end)) {
      return 0;
    }

    values_out[count++] = atoi(string_val + offset_begin);
    offset_begin += offset_end + 1; // skip delimiter
  }

  return count == expected_count;
}

int parse_array_u8(const char *string_val, size_t string_len,
                   size_t expected_count, uint8_t *values_out) {
  size_t offset_begin = 0;
  size_t offset_end = 0;
  size_t count = 0;

  while (count < expected_count && offset_begin < string_len) {
    if (!offset_to_char(string_val + offset_begin, string_len - offset_begin,
                        PARSER_DELIMITER_VALUE, &offset_end)) {
      return 0;
    }

    int tmp = atoi(string_val + offset_begin);
    if (tmp < 0 || tmp > 255) {
      LOG_WARN("Value %d does not fit in u8", tmp);
      return 0;
    }
    values_out[count++] = (uint8_t)tmp;
    offset_begin += offset_end + 1; // skip delimiter
  }

  return count == expected_count;
}

void test_parser() {
  const char *data =
      "size_world=25,20,\nsize_tile_map=10,3,\ntilemap_texture=jungle.png\n";
  Buffer buffer;
  buffer.data = (uint8_t *)data;
  buffer.used = strlen(data);
  buffer.capacity = buffer.used;

  ParseState parser = {.buffer = &buffer, .offset = 0};
  size_t num_parsed = 0;
  while (!parse_is_done(&parser)) {
    KeyValueRaw raw;
    Assert(get_next_key_value_raw(&parser, &raw) != 0);

    if (strncmp(raw.key_begin, "size_world", strlen("size_world")) == 0) {
      int values[2] = {0};
      Assert(parse_array_int(raw.value_begin, raw.value_len, 2, values) != 0);
      Assert(values[0] == 25);
      Assert(values[1] == 20);
      ++num_parsed;
    } else if (strncmp(raw.key_begin, "size_tile_map",
                       strlen("size_tile_map")) == 0) {
      int values[2] = {0};
      Assert(parse_array_int(raw.value_begin, raw.value_len, 2, values) != 0);
      Assert(values[0] == 10);
      Assert(values[1] == 3);
      ++num_parsed;
    } else if (strncmp(raw.key_begin, "tilemap_texture",
                       strlen("tilemap_texture")) == 0) {
      Assert(strncmp(raw.value_begin, "jungle.png", raw.value_len) == 0);
      ++num_parsed;
    }
  }
  Assert(num_parsed == 3);
}
#endif
