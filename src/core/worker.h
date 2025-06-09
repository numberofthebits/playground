#ifndef WORKER_H
#define WORKER_H

#include "os.h"

#include <stdalign.h>
#include <threads.h>

#define WORK_QUEUE_MAX_SIZE 100000
#define WORKER_THREAD_COUNT 8
#define NUM_THREADS (1 + WORKER_THREAD_COUNT)

typedef void (*JobFn)(void *);

struct WorkQueue;

typedef struct {
  thrd_t thread_handle;
  SemaphoreHandle semaphore_handle;
  int thread_index;
  struct WorkQueue *queue;
} WorkerThread;

typedef struct {
  JobFn job_fn;
  void *job_args;
} WorkQueueEntry;

struct WorkQueue {
  WorkQueueEntry work_queue[WORK_QUEUE_MAX_SIZE];
  SemaphoreHandle semaphore_handle;
  WorkerThread worker_threads[WORKER_THREAD_COUNT];

  // Producer writes (resets to zero). Consumers read and write.
  volatile uint32_t work_queue_next;

  // Procuder writes and reads. Consumers read
  volatile uint32_t work_queue_count;

  // Producer reads. Consumers write.
  volatile uint32_t work_queue_completed;
};

// Returns 0 on failure
int work_queue_init(struct WorkQueue *queue);

int work_queue_push(struct WorkQueue *queue, JobFn func, void *args);

void work_queue_commit(struct WorkQueue *queue);

// Run until all work is completed
void work_queue_sync(struct WorkQueue *queue);

#ifdef BUILD_TESTS
void work_queue_test();
#endif

#endif
