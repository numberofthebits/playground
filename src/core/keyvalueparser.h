#ifndef KEYVALUEPARSER
#define KEYVALUEPARSER

#include "util.h"

#include "log.h"

#define PARSER_DELIMITER_KEY_VALUE '='
#define PARSER_DELIMITER_VALUE ','
#define PARSER_DELIMITER_KEY_VALUE_PAIR '\n'

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

typedef int (*ParseCallback)(KeyValueRaw *);

int parse_buffer(Buffer *buffer, ParseCallback callback);

// Values out must be able to hold 'expected_count'
int parse_int_array(const char *string_val, size_t string_len,
                    size_t expected_count, int *values_out);

#ifdef BUILD_TESTS
int test_parser_callback(KeyValueRaw *raw);
void test_parser();
#endif

#endif
