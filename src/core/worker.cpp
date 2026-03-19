#include "worker.h"

#include "allocators.h"
#include "log.h"
#include "os.h"

#include <cstdio>

#define WORK_QUEUE_PROCESS_RESULT_AGAIN 0
#define WORK_QUEUE_PROCESS_RESULT_DONE 1

// Returns non-zero if there is no more work to do,
// and zero if it either did work, or it couldn't determine
// what to do, i.e. some caller picked up next unit of work
// before we could
// static int work_queue_process(WorkerThread *thread) {

//   uint32_t original_next_queue_index = thread->work_queue_next;
//   if (original_next_queue_index < thread->work_queue_count) {

//     // TODO: Replace with something general
//     uint32_t actual_index = InterlockedCompareExchange(
//         (LONG volatile *)&thread->work_queue_next,
//         original_next_queue_index + 1, original_next_queue_index);

//     if (actual_index == original_next_queue_index) {
//       // Pull out our unit of work
//       WorkQueueEntry *entry = &thread->work_queue[actual_index];

//       // LOG_INFO("Thread %d doing work entry %d", thread->thread_index,
//       //          actual_index);
//       entry->job_fn(entry->job_args);

//       // TODO: Replace with something general
//       InterlockedIncrement(
//           (LONG volatile *)&thread->queue->work_queue_completed);
//     }
//   } else {
//     // LOG_INFO("Thread %d no more work to do %d/%d", thread_index,
//     //        original_next_queue_index, work_queue_next);
//     return WORK_QUEUE_PROCESS_RESULT_DONE;
//   }

//   return WORK_QUEUE_PROCESS_RESULT_AGAIN;
// }

static void work_queue_process(WorkerThread *thread) {

  while (!thread->work_queue.empty()) {
    WorkQueueEntry *entry = thread->work_queue.head();

    // LOG_INFO("Thread %d doing work entry %d", thread->thread_index,
    //          actual_index);
    entry->job_fn(entry->job_args);

    thread->work_queue.pop();

    // TODO: Replace with something general
    InterlockedIncrement((LONG volatile *)&thread->queue->work_queue_completed);
  }
}

static int worker_thread_main(void *arg) {
  WorkerThread *args = (WorkerThread *)arg;
  thread_name = args->thread_name;
  LOG_INFO("Thread %s (%d) started", args->thread_name, args->thread_index);

  // We could, if WorkQueue is only constructed once in the application lifetime,
  // not query the running state at all
  while(args->want_running) {
    work_queue_process(args);
    // LOG_INFO("Thread %d going to sleep", args.thread_index);
    WaitForSemaphore(args->semaphore_handle);
    // LOG_INFO("Thread %d woke up", args.thread_index);

    // if (work_queue_process(args) == WORK_QUEUE_PROCESS_RESULT_DONE) {
    //   // LOG_INFO("Thread %d going to sleep", args.thread_index);
    //   WaitForSemaphore(args->semaphore_handle);
    //   // LOG_INFO("Thread %d woke up", args.thread_index);
    // }
  }

  LOG_INFO("Thread %s (%d) done", args->thread_name, args->thread_index);

  return 0;
}

static void work_queue_reset(struct WorkQueue *queue) {

  for (size_t i = 0; i < WORKER_THREAD_COUNT; ++i) {
    InterlockedExchange(
        (LONG *)&queue->worker_threads[i].work_queue_count, 0);

    InterlockedExchange(
        (LONG *)&queue->worker_threads[i].work_queue_next, 0);
  }

  InterlockedExchange((LONG *)&queue->work_queue_completed, 0);
  queue->next_worker_thread = 0;
  queue->work_queue_queued_total = 0;

}

int work_queue_init(struct WorkQueue *queue) {
  work_queue_reset(queue);

  LOG_INFO("Creating %d worker threads", WORKER_THREAD_COUNT);

  for (size_t i = 0; i < WORKER_THREAD_COUNT; ++i) {
    auto thread_index = int(i + 1);
    // Reserve thread_index 0 for our main thread

    queue->worker_threads[i].semaphore_handle =
        MakeSemaphore(WORKER_THREAD_COUNT);

    if (queue->worker_threads[i].semaphore_handle == NULL) {
      LOG_ERROR("Failed to create work queue semaphore");
      return 0;
    }

    sprintf(queue->worker_threads[i].thread_name, "WorkerPoolThread %d",
            thread_index);
    
    queue->worker_threads[i].work_queue.init();
    queue->worker_threads[i].thread_index = thread_index;
    queue->worker_threads[i].queue = queue;
    queue->worker_threads[i].want_running = true;
    
    queue->worker_threads[i].thread_handle = std::thread([thread_args = &queue->worker_threads[i]]() {
      worker_thread_main(thread_args);
    });
    // if (thrd_create(&queue->worker_threads[i].thread_handle,
    //                 &worker_thread_main,
    //                 &queue->worker_threads[i]) != thrd_success) {
    //   LOG_ERROR("Failed to create thread %s %d",
    //             queue->worker_threads[i].thread_name);
    //   return 0;
    // }
  }

  return 1;
}

static inline void work_queue_signal_worker_thread(WorkerThread *thread) {
  /* if (SignalSemaphore(queue->semaphore_handle, */
  /*                     queue->work_queue_count > WORKER_THREAD_COUNT */
  /*                         ? WORKER_THREAD_COUNT */
  /*                         : queue->work_queue_count) == FALSE) { */

  if (SignalSemaphore(thread->semaphore_handle, 1) == FALSE) {
    LOG_WARN("Failed to signal semaphore");
  }
}

int work_queue_push(struct WorkQueue *queue, JobFn func, void *args) {
  if (queue->work_queue_queued_total >= WORK_QUEUE_MAX_SIZE) {
    LOG_WARN("Work queue full (%d)", WORK_QUEUE_MAX_SIZE);
    return 0;
  }

  uint32_t this_task_thread_index = queue->next_worker_thread;
  WorkerThread *thread = &queue->worker_threads[this_task_thread_index];
  queue->next_worker_thread = ++queue->next_worker_thread % WORKER_THREAD_COUNT;

  //  WorkQueueEntry *entry = &thread->work_queue[thread->work_queue_count];
  WorkQueueEntry entry;
  entry.job_fn = func;
  entry.job_args = args;
  thread->work_queue.push(entry);

  //  LOG_INFO("Push work entry %d", queue->work_queue_count);

  CompletePastWritesBeforeFutureWrites();
  //++queue->work_queue_count;
  // TODO: This was the one i tried changing to see if it affected
  // the first batch of work not being processed.
  // It SHOULD not matter. Only main thread writes to work_queue_count
  InterlockedIncrement((LONG volatile *)&thread->work_queue_count);
  ++queue->work_queue_queued_total;

  work_queue_signal_worker_thread(thread);

  return 1;
}

/* void work_queue_commit(struct WorkQueue *queue) { */
/*   CompletePastWritesBeforeFutureWrites(); */
/*   work_queue_signal_worker_thread(queue); */
/* } */

void work_queue_sync(struct WorkQueue *queue) {
  while (queue->work_queue_completed != queue->work_queue_queued_total) {
    _mm_pause();
  }
  // CompletePastWritesBeforeFutureWrites();
  // CompletePastReadsBeforeFutureReads();
  work_queue_reset(queue);
}

#ifdef BUILD_TESTS
void print_string(void *args) { LOG_INFO("%s", (const char *)args); }

void work_queue_test() {
  struct WorkQueue *queue = new WorkQueue();

  if (!work_queue_init(queue)) {
    LOG_EXIT("Failed to init work queue");
  }
  LOG_INFO("Queue jobs");

  work_queue_push(queue, &print_string, (void *)"hello a");
  work_queue_push(queue, &print_string, (void *)"hello b");
  work_queue_push(queue, &print_string, (void *)"hello c");
  work_queue_push(queue, &print_string, (void *)"hello d");
  work_queue_push(queue, &print_string, (void *)"hello e");
  work_queue_push(queue, &print_string, (void *)"hello f");
  work_queue_push(queue, &print_string, (void *)"hello g");
  work_queue_push(queue, &print_string, (void *)"hello h");
  work_queue_push(queue, &print_string, (void *)"hello j");

  work_queue_sync(queue);

  delete queue;
}
#endif
