
#ifdef BUILD_TESTS
#include "ringbuf.h"
#include "util.h"

void test_ringbuf() {
  RingBuffer<int, 3> buffer;
  buffer.init();
  Assert(buffer.size() == 0);
  Assert(buffer.empty());

  buffer.push(1);
  Assert(buffer.size() == 1);
  Assert(!buffer.empty());

  buffer.push(2);
  buffer.push(3);

  Assert(buffer.size() == 3);

  auto iter = buffer.begin();
  Assert(*iter == 1);
  ++iter;
  Assert(*iter == 2);
  ++iter;
  Assert(*iter == 3);

  int count = 0;
  for (auto &v : buffer) {
    Assert(v == count + 1);
    count++;
  }

  Assert(count == 3);

  for (int i = 0; i < 10; ++i) {
    buffer.pop();
    buffer.push(i + 10);
    for (auto v : buffer) {
      LOG_INFO(" ringbuf value %d", v);
    }
  }
}
#endif
