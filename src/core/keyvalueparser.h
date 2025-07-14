#ifndef KEYVALUEPARSER
#define KEYVALUEPARSER

#include "util.h"

#include "log.h"

#define PARSER_DELIMITER_KEY_VALUE '='
#define PARSER_DELIMITER_VALUE ','
#define PARSER_DELIMITER_KEY_VALUE_PAIR '\n'
#define PARSER_OK 1
#define PARSER_ERROR -1
#define PARSER_DONE 0

typedef struct KeyValueRaw {
  const char *key_begin;
  size_t key_len;
  const char *value_begin;
  size_t value_len;
} KeyValueRaw;

typedef struct ParseState {
  Buffer *buffer;
  size_t offset;
} ParseState;

// Returns PARSER_DONE, PARSER_ERROR or PARSER_OK
int get_next_key_value_raw(ParseState *state, KeyValueRaw *raw);

static inline int parse_is_done(ParseState *state) {
  return state->offset >= state->buffer->used;
}

// Values out must be able to hold 'expected_count'
int parse_array_int(const char *string_val, size_t string_len,
                    size_t expected_count, int *values_out);

int parse_array_u8(const char *str_val, size_t str_len, size_t expected_count,
                   uint8_t *values_out);

#ifdef BUILD_TESTS
void test_parser();
#endif

#endif
