#pragma once

#include "log.h"

#include <atomic>
#include <iterator>
#include <stdint.h>
#include <utility>

template <typename T, size_t S> struct RingBuffer {
  struct RingBufferIterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = value_type *;
    using reference = value_type &;

    inline RingBufferIterator(RingBuffer &buf, uint32_t index_, uint32_t count)
        : m_buf(buf), m_index(index_), m_count(count) {}

    inline reference operator*() const { return m_buf.values[m_index]; }

    inline pointer operator->() { return &m_buf.values[m_index]; }

    inline RingBufferIterator &operator++() {
      m_index = ++m_index % m_buf.m_size;
      ++m_count;
      return *this;
    }

    inline RingBufferIterator operator++(int) {
      RingBufferIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend inline bool operator==(const RingBufferIterator &a,
                                  const RingBufferIterator &b) {
      return a.m_index == b.m_index && a.m_count == b.m_count;
    }

    friend inline bool operator!=(const RingBufferIterator &a,
                                  const RingBufferIterator &b) {
      return !operator==(a, b);
    }

    RingBuffer &m_buf;
    uint32_t m_index;
    uint32_t m_count;
  };

  /*alignas(64)*/ uint32_t m_index_read;
  /*alignas(64)*/ std::atomic<uint32_t> m_index_write;
  /*alignas(64)*/ std::atomic<uint32_t> m_size;

  T values[S];

  static_assert(std::atomic<uint32_t>::is_always_lock_free);

  inline void init() {
    m_index_read = 0;
    m_index_write = 0;
    m_size = 0;
  }

  inline uint32_t size() const { return m_size; }
  inline uint32_t capacity() const { return S; }
  inline uint32_t empty() const { return !m_size; }

  inline void pop() {
    if (m_size == 0) {
      LOG_EXIT("ringbuf pop with empty buffer");
      return;
    }

    this->m_index_read = ++this->m_index_read % S;
    this->m_size--;
  }

  inline T *head() {
    if (this->empty()) {
      return nullptr;
    }

    return &this->values[this->m_index_read];
  }

  inline void push(T value) {
    if (m_size == S) {
      LOG_EXIT("ringbuf push with full buffer");
      return;
    }

    this->values[m_index_write] = std::move(value);
    this->m_index_write = ++this->m_index_write % S;
    this->m_size++;
  }

  inline RingBufferIterator begin() {
    return RingBufferIterator(*this, m_index_read, 0);
  }

  inline RingBufferIterator end() {
    return RingBufferIterator(*this, m_index_write, m_size);
  }
};

#ifdef BUILD_TESTS
void test_ringbuf();
#endif
