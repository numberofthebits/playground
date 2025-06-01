#ifndef WORKER_H
#define WORKER_H

#include <threads.h>

typedef void (*JobFn)(void *);

typedef struct {
  JobFn job_fn;
  void *job_args;
} WorkQueueEntry;

typedef struct {
  int index;
} WorkerThreadArgs;

// Returns 0 on failure
int work_queue_init();

void work_queue_push(JobFn func, void *args);

#ifdef BUILD_TESTS
void work_queue_test();
#endif

#endif
