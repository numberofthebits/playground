#ifndef STATISTICS_H
#define STATISTICS_H

#include "log.h"

#include <cstdint>
#include <cstring>

#define STATISTICS_CIRCULAR_BUFFER_SIZE 64
#define STATISTICS_DURATIONS_MAX 128
#define STATISTICS_DURATION_FLAG_TEXT 1
#define STATISTICS_DURATION_FLAG_PLOT 2
#define STATISTICS_SERIES_MAX 128

typedef struct Duration {
  uint64_t value;
  const char *name;
} Duration;

struct CircularBuffer {
  uint64_t values[STATISTICS_CIRCULAR_BUFFER_SIZE];
  uint32_t index_read;
  uint32_t index_write;
  uint32_t size;
  const char *name;
};

static inline void cbuf_init(CircularBuffer *buffer) {
  buffer->size = 0;
  buffer->index_read = 0;
  buffer->index_write = 0;
  buffer->size = 0;

  memset(buffer->values, 0x0, sizeof(buffer->values));
}

static inline void cbuf_pop(CircularBuffer *buffer) {
  if (buffer->size == 0) {
    LOG_WARN("cbuf pop with empty buffer");
    return;
  }

  buffer->index_read = ++buffer->index_read % STATISTICS_CIRCULAR_BUFFER_SIZE;
  buffer->size--;
}

static inline void cbuf_push(CircularBuffer *buffer, uint64_t value) {
  if (buffer->size == STATISTICS_CIRCULAR_BUFFER_SIZE) {
    cbuf_pop(buffer);
    /* LOG_WARN("cbuf push with full buffer"); */
    /* return; */
  }

  buffer->values[buffer->index_write] = value;
  buffer->index_write = ++buffer->index_write % STATISTICS_CIRCULAR_BUFFER_SIZE;
  buffer->size++;
}

static inline uint64_t cbuf_next(CircularBuffer *buffer, uint32_t index_prev) {
  return (index_prev + 1) % (buffer->size < STATISTICS_CIRCULAR_BUFFER_SIZE
                                 ? buffer->size
                                 : STATISTICS_CIRCULAR_BUFFER_SIZE);
}

typedef struct Statistics {
  uint32_t num_durations;
  uint32_t num_series;

  Duration durations[STATISTICS_DURATIONS_MAX];
  CircularBuffer series[STATISTICS_SERIES_MAX];
} Statistics;

extern Statistics stats;

Duration *statistics_reserve_duration(const char *name);

CircularBuffer *statistics_reserve_series(const char *name);

#ifdef BUILD_TESTS
void test_statistics();
#endif

#endif
