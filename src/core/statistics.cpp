#include "statistics.h"

#include "log.h"
Statistics stats;

Duration *statistics_reserve_entry(const char *name) {

  if (stats.count == STATISTICS_DURATIONS_MAX) {
    return nullptr;
  }
  Duration *dur = &stats.durations[stats.count++];
  dur->name = name;
  dur->value = 0;

  return dur;
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
    index = cbuf_next(index);
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

  // for (uint64_t *iter_ = cbuf_begin(&buf); iter_ != cbuf_end(&buf);
  //      cbuf_next(&buf, iter_)) {
  //   LOG_INFO("%llu", *iter_);
  //   ++iter_;
  // }
}
#endif
