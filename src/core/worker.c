#include "worker.h"

#include "log.h"
#include "os.h"

typedef struct {
  thrd_t thread_handle;
  SemaphoreHandle semaphore_handle;
  int thread_index;
} WorkerThread;

static SemaphoreHandle semaphore_handle;
static WorkerThread worker_threads[WORKER_THREAD_COUNT];
static volatile uint32_t work_queue_next = 0;
static volatile uint32_t work_queue_count = 0;
static volatile uint32_t work_queue_completed = 0;
static WorkQueueEntry work_queue[256];

static inline int process_work_queue(int thread_index) {
  int did_work = 0;
  while (work_queue_next < work_queue_count) {
    // TODO: Replace InterlockedIncrement with something general
    int entry_index =
        InterlockedIncrement((LONG volatile *)&work_queue_next) - 1;

    CompletePastReadsBeforeFutureReads();

    // Pull out our unit of work
    WorkQueueEntry *entry = &work_queue[entry_index];

    LOG_INFO("Thread %d doing work", thread_index);
    entry->job_fn(entry->job_args);

    // TODO: Replace InterlockedIncrement with something general
    // Notify main thread that we completed a unit of work (not which
    // unit)
    InterlockedIncrement((LONG volatile *)&work_queue_completed);
    did_work &= 0x1;
  }

  return did_work;
}

static int worker_thread_main(void *arg) {
  WorkerThread args = *(WorkerThread *)arg;
  LOG_INFO("Thread %d started", args.thread_index);

  // struct timespec ts = {.tv_sec = 1};

  for (;;) {
    if (!process_work_queue(args.thread_index)) {
      WaitForSemaphore(args.semaphore_handle);
    }
  }

  LOG_INFO("Thread %d done", args.thread_index);
}

int work_queue_init() {
  semaphore_handle = MakeSemaphore();

  if (semaphore_handle == NULL) {
    LOG_ERROR("Failed to create work queue semaphore");
    return 0;
  }

  LOG_INFO("Creating %d worker threads", WORKER_THREAD_COUNT);

  for (size_t i = 0; i < WORKER_THREAD_COUNT; ++i) {
    // Reserve thread_index 0 for our main thread
    worker_threads[i].thread_index = i + 1;
    worker_threads[i].semaphore_handle = semaphore_handle;

    if (thrd_create(&worker_threads[i].thread_handle, &worker_thread_main,
                    &worker_threads[i]) != thrd_success) {
      LOG_ERROR("Failed to create thread %d", i);
      return 0;
    }
  }

  return 1;
}

void work_queue_push(JobFn func, void *args) {
  WorkQueueEntry *entry = &work_queue[work_queue_count];
  entry->job_fn = func;
  entry->job_args = args;
  ++work_queue_count;

  SignalSemaphore(semaphore_handle);
}

#ifdef BUILD_TESTS
void print_string(void *args) { LOG_INFO("%s", (const char *)args); }

/* void push_string(SemaphoreHandle semaphore_handle, const char *str) { */
/*   WorkQueueEntry *entry = &work_queue[work_queue_count]; */
/*   entry->to_print = str; */
/*   ++work_queue_count; */

/*   SignalSemaphore(semaphore_handle); */
/* } */

void work_queue_test() {

  if (!work_queue_init()) {
    LOG_EXIT("Failed to init work queue");
  }

  LOG_INFO("Queue jobs");
  work_queue_push(&print_string, "hello a");
  work_queue_push(&print_string, "hello b");
  work_queue_push(&print_string, "hello c");
  work_queue_push(&print_string, "hello d");
  work_queue_push(&print_string, "hello e");
  work_queue_push(&print_string, "hello f");
  work_queue_push(&print_string, "hello g");
  work_queue_push(&print_string, "hello h");

  while (work_queue_completed != work_queue_count) {
    process_work_queue(0);
  }
}
#endif
