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

static int get_key_value_raw(ParseState *state, KeyValueRaw *raw) {

  const char *begin = (const char *)state->buffer->data + state->offset;
  *raw = (KeyValueRaw){0};
  size_t line_len = 0;
  if (!offset_to_char(begin, state->buffer->len,
                      PARSER_DELIMITER_KEY_VALUE_PAIR, &line_len)) {
    LOG_WARN("Failed to find line length");
    return 0;
  }

  if (!line_len) {
    LOG_WARN("0 line length");
    return 0;
  }

  size_t key_len = 0;
  if (!offset_to_char(begin, line_len, PARSER_DELIMITER_KEY_VALUE, &key_len)) {
    LOG_WARN("Failed to find key length");
    return 0;
  }

  size_t value_len = line_len - key_len - 1;

  raw->key_begin = begin;
  raw->key_len = key_len;
  raw->value_begin = begin + key_len + 1;
  raw->value_len = value_len;

  state->offset += line_len + 1; // skip newline

  return 1;
}

int parse_buffer(Buffer *buffer, ParseCallback callback) {
  ParseState state = {.buffer = buffer, .offset = 0};
  size_t last_offset = state.offset;
  while (state.offset < state.buffer->len) {
    KeyValueRaw raw = {0};
    if (!get_key_value_raw(&state, &raw)) {
      return 0;
    }

    if (last_offset == state.offset) {
      LOG_WARN("Failed to make progress");
      return 0;
    }

    last_offset = state.offset;

    if (!callback(&raw)) {
      LOG_WARN("Callback aborted parsing");
      return 0;
    }
  }

  return 1;
}

#ifdef BUILD_TESTS
// Values out must be able to hold 'expected_count'
int parse_int_array(const char *string_val, size_t string_len,
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

int test_parser_callback(KeyValueRaw *raw) {
  printf("key=%.*s value=%.*s\n", (int)raw->key_len, raw->key_begin,
         (int)raw->value_len, raw->value_begin);
  if (strncmp(raw->key_begin, "size_world", strlen("size_world")) == 0) {
    int values[2] = {0};
    if (!parse_int_array(raw->value_begin, raw->value_len, 2, values)) {
      return 0;
    }
    Assert(values[0] == 25);
    Assert(values[1] == 20);
  } else if (strncmp(raw->key_begin, "size_tile_map",
                     strlen("size_tile_map")) == 0) {
    int values[2] = {0};
    if (!parse_int_array(raw->value_begin, raw->value_len, 2, values)) {
      return 0;
    }
    Assert(values[0] == 10);
    Assert(values[1] == 3);
  } else if (strncmp(raw->key_begin, "tilemap_texture",
                     strlen("tilemap_texture")) == 0) {
    Assert(strncmp(raw->value_begin, "jungle.png", strlen("jungle.png")) == 0);
  }
  return 1;
}
void test_parser() {
  const char *data =
      "size_world=25,20,\nsize_tile_map=10,3,\ntilemap_texture=jungle.png\n";
  Buffer buffer;
  buffer.data = (uint8_t *)data;
  buffer.len = strlen(data);

  Assert(parse_buffer(&buffer, &test_parser_callback) != 0);
}
#endif
