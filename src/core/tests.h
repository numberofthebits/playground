#ifndef TESTS_H
#define TESTS_H
#ifdef BUILD_TESTS

#include "allocators.h"
#include "ecs.h"
#include "keyvalueparser.h"
#include "math.h"
#include "sparsecomponentpool.h"
#include "worker.h"

static inline void run_tests() {

  arena_init(&global_static_allocator, 1024 * 1024 * 32);
  stack_init(&stack_allocator, &global_static_allocator, 1024 * 1024 * 8);
  test_parser();
  //  assetstore_test();
  /* registry_test(); */
  /* work_queue_test(); */
  /* math_test(); */
  /* pool_test(); */

  arena_free(&global_static_allocator);
}
#endif // BUILD_TESTS
#endif // TESTS_H
