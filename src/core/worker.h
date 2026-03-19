#ifndef WORKER_H
#define WORKER_H

#include "os.h"
#include "ringbuf.h"

#include <thread>

#define WORK_QUEUE_MAX_SIZE 10000
#define WORKER_THREAD_COUNT 7
#define NUM_THREADS (1 + WORKER_THREAD_COUNT)

typedef void (*JobFn)(void *);

struct WorkQueue;

typedef struct {
  JobFn job_fn;
  void *job_args;
} WorkQueueEntry;

struct WorkerThread {
  WorkerThread()
    : want_running(false) {
  }
  //  WorkQueueEntry work_queue[WORK_QUEUE_MAX_SIZE];
  RingBuffer<WorkQueueEntry, 1024> work_queue;
  std::thread thread_handle;
  std::atomic<bool> want_running;
  SemaphoreHandle semaphore_handle;
  struct WorkQueue *queue;
  int thread_index;

  // Keep these counters cache aligned (assuming 64 byte cache lines)
  // to avoid false sharing.
  
  // Producer writes (resets to zero). Consumers read and write.
  alignas(64) volatile uint32_t work_queue_next;

  // Producer writes and reads. Consumers read
  alignas(64) volatile uint32_t work_queue_count;

  char thread_name[64];
};

struct WorkQueue {
  WorkerThread worker_threads[WORKER_THREAD_COUNT];

  uint32_t next_worker_thread;

  // Producer reads. Consumers write.
  alignas(64) volatile uint32_t work_queue_completed;

  uint32_t work_queue_queued_total;

  ~WorkQueue() {
    for (auto& wt : worker_threads) {
      wt.want_running = false;
      if (SignalSemaphore(wt.semaphore_handle, 1) == FALSE) {
        LOG_WARN("Failed to signal semaphore");

      } else {
        wt.thread_handle.join();
      }                      
    }
  }
};

// Returns 0 on failure
int work_queue_init(struct WorkQueue *queue);

int work_queue_push(struct WorkQueue *queue, JobFn func, void *args);

/* void work_queue_commit(struct WorkQueue *queue); */

// Run until all work is completed
void work_queue_sync(struct WorkQueue *queue);

#ifdef BUILD_TESTS
void work_queue_test();
#endif

#endif
