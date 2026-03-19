#include "statistics.h"

#include "log.h"

Statistics stats;

Duration *statistics_reserve_duration(const char *name) {

  if (stats.num_durations == STATISTICS_DURATIONS_MAX) {
    return nullptr;
  }
  Duration *dur = &stats.durations[stats.num_durations++];
  dur->name = name;
  dur->value = 0;

  return dur;
}

CircularBuffer *statistics_reserve_series(const char *name) {
  if (stats.num_series == STATISTICS_SERIES_MAX) {
    return nullptr;
  }
  CircularBuffer *buffer = &stats.series[stats.num_series++];

  cbuf_init(buffer);

  buffer->name = name;

  return buffer;
}
#ifdef BUILD_TESTS
static inline void log_cbuf(CircularBuffer *buffer) {
  for (uint32_t i = 0; i < STATISTICS_CIRCULAR_BUFFER_SIZE; ++i) {
    LOG_INFO("pos %d value %llu", buffer->values[i]);
  }
}

void test_statistics() {
  CircularBuffer buf;
  cbuf_init(&buf);

  for (uint32_t i = 0; i < STATISTICS_CIRCULAR_BUFFER_SIZE + 1; ++i) {
    cbuf_push(&buf, (uint64_t)i);
  }

  for (uint32_t i = 0; i < STATISTICS_CIRCULAR_BUFFER_SIZE + 1; ++i) {
    cbuf_pop(&buf);
  }

  uint32_t index = buf.index_read;
  for (uint32_t i = 0; i < buf.size; ++i) {
    LOG_INFO("%d %llu ", i, buf.values[index]);
    index = cbuf_next(&buf, index);
  }

  for (int i = 0; i < STATISTICS_CIRCULAR_BUFFER_SIZE; ++i) {
    cbuf_push(&buf, (uint64_t)i);
  }

  for (int i = 0; i < 4; ++i) {
    LOG_INFO("Pop and push new value");
    cbuf_pop(&buf);
    cbuf_push(&buf, 10 + i);
    log_cbuf(&buf);
  }
}
#endif
