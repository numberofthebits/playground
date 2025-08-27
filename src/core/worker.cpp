#include "worker.h"

#include "log.h"
#include "os.h"

#define WORK_QUEUE_PROCESS_RESULT_AGAIN 0
#define WORK_QUEUE_PROCESS_RESULT_DONE 1

// Returns non-zero if there is no more work to do,
// and zero if it either did work, or it couldn't determine
// what to do, i.e. some caller picked up next unit of work
// before we could
static int work_queue_process(struct WorkQueue *queue, int thread_index) {

  uint32_t original_next_queue_index = queue->work_queue_next;
  if (original_next_queue_index < queue->work_queue_count) {

    // TODO: Replace with something general
    uint32_t actual_index = InterlockedCompareExchange(
        (LONG volatile *)&queue->work_queue_next, original_next_queue_index + 1,
        original_next_queue_index);

    if (actual_index == original_next_queue_index) {
      // Pull out our unit of work
      WorkQueueEntry entry = queue->work_queue[actual_index];

      LOG_INFO("Thread %d doing work entry %d", thread_index, actual_index);
      entry.job_fn(entry.job_args);

      // TODO: Replace with something general
      InterlockedIncrement((LONG volatile *)&queue->work_queue_completed);
    }
  } else {
    // LOG_INFO("Thread %d no more work to do %d/%d", thread_index,
    //        original_next_queue_index, work_queue_next);
    return WORK_QUEUE_PROCESS_RESULT_DONE;
  }

  return WORK_QUEUE_PROCESS_RESULT_AGAIN;
}

static int worker_thread_main(void *arg) {
  WorkerThread args = *(WorkerThread *)arg;
  LOG_INFO("Thread %d started", args.thread_index);

  for (;;) {
    if (work_queue_process(args.queue, args.thread_index) ==
        WORK_QUEUE_PROCESS_RESULT_DONE) {
      // LOG_INFO("Thread %d going to sleep", args.thread_index);
      WaitForSemaphore(args.semaphore_handle);
      // LOG_INFO("Thread %d woke up", args.thread_index);
    }
  }

  LOG_INFO("Thread %d done", args.thread_index);

  return 0;
}

static void work_queue_reset(struct WorkQueue *queue) {
  InterlockedExchange((LONG *volatile)&queue->work_queue_next, 0);
  InterlockedExchange((LONG *volatile)&queue->work_queue_count, 0);
  InterlockedExchange((LONG *volatile)&queue->work_queue_completed, 0);
}

int work_queue_init(struct WorkQueue *queue) {
  work_queue_reset(queue);
  queue->semaphore_handle = MakeSemaphore(WORKER_THREAD_COUNT);

  if (queue->semaphore_handle == NULL) {
    LOG_ERROR("Failed to create work queue semaphore");
    return 0;
  }

  LOG_INFO("Creating %d worker threads", WORKER_THREAD_COUNT);

  for (size_t i = 0; i < WORKER_THREAD_COUNT; ++i) {
    // Reserve thread_index 0 for our main thread
    queue->worker_threads[i].thread_index = i + 1;
    queue->worker_threads[i].semaphore_handle = queue->semaphore_handle;
    queue->worker_threads[i].queue = queue;

    if (thrd_create(&queue->worker_threads[i].thread_handle,
                    &worker_thread_main,
                    &queue->worker_threads[i]) != thrd_success) {
      LOG_ERROR("Failed to create thread %d", i);
      return 0;
    }
  }

  return 1;
}

static inline void work_queue_signal_worker_thread(struct WorkQueue *queue) {
  /* if (SignalSemaphore(queue->semaphore_handle, */
  /*                     queue->work_queue_count > WORKER_THREAD_COUNT */
  /*                         ? WORKER_THREAD_COUNT */
  /*                         : queue->work_queue_count) == FALSE) { */
  if (SignalSemaphore(queue->semaphore_handle, 1) == FALSE) {

    LOG_WARN("Failed to signal semaphore");
  }
}

int work_queue_push(struct WorkQueue *queue, JobFn func, void *args) {
  if (queue->work_queue_count >= WORK_QUEUE_MAX_SIZE) {
    LOG_WARN("Work queue full (%d)", WORK_QUEUE_MAX_SIZE);
    return 0;
  }

  WorkQueueEntry *entry = &queue->work_queue[queue->work_queue_count];
  entry->job_fn = func;
  entry->job_args = args;

  //  LOG_INFO("Push work entry %d", queue->work_queue_count);

  CompletePastWritesBeforeFutureWrites();
  //++queue->work_queue_count;
  // TODO: This was the one i tried changing to see if it affected
  // the first batch of work not being processed.
  // It SHOULD not matter. Only main thread writes to work_queue_count
  InterlockedIncrement((LONG volatile *)&queue->work_queue_count);
  work_queue_signal_worker_thread(queue);
  return 1;
}

/* void work_queue_commit(struct WorkQueue *queue) { */
/*   CompletePastWritesBeforeFutureWrites(); */
/*   work_queue_signal_worker_thread(queue); */
/* } */

void work_queue_sync(struct WorkQueue *queue) {
  while (queue->work_queue_completed != queue->work_queue_count) {
    work_queue_process(queue, 0);
    //_mm_pause();
  }

  LOG_INFO("completed=%d count=%d next=%d", queue->work_queue_completed,
           queue->work_queue_count, queue->work_queue_next);
  /* queue->work_queue_next = 0; */
  /* queue->work_queue_count = 0; */
  /* queue->work_queue_completed = 0; */

  work_queue_reset(queue);
}

#ifdef BUILD_TESTS
void print_string(void *args) { LOG_INFO("%s", (const char *)args); }

void work_queue_test() {
  struct WorkQueue *queue = malloc(sizeof(struct WorkQueue));

  if (!work_queue_init(queue)) {
    LOG_EXIT("Failed to init work queue");
  }
  Sleep(5000);
  LOG_INFO("Queue jobs");

  work_queue_push(queue, &print_string, "hello a");
  work_queue_push(queue, &print_string, "hello b");
  work_queue_push(queue, &print_string, "hello c");
  work_queue_push(queue, &print_string, "hello d");
  work_queue_push(queue, &print_string, "hello e");
  work_queue_push(queue, &print_string, "hello f");
  work_queue_push(queue, &print_string, "hello g");
  work_queue_push(queue, &print_string, "hello h");
  work_queue_push(queue, &print_string, "hello j");

  work_queue_sync(queue);
}
#endif
