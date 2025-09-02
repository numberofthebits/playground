#ifndef OS_H
#define OS_H

#include <stdint.h>

#define FILE_CALLBACK_RESULT_CONTINUE 0
#define FILE_CALLBACK_RESULT_STOP 1

typedef struct {
  const char *file_path;
  const char *file_name;
  const char *extension;
} FileSystemListResult;

typedef int (*IterCallback)(const FileSystemListResult *result, void *context);

#ifdef _WIN32
#include <intrin.h>
#include <windows.h>
typedef LARGE_INTEGER TimeT;
#define CALLING_CONVENTION APIENTRY

#elif __linux__
#include <time.h>
typedef struct timespec TimeT;
#define CALLING_CONVENTION
#else
#error "What is TimeT on this platform?"
#endif

#include "log.h"

// Ask file system to list contents of 'directory'. Optionally filter by
// wildcards
void file_system_list(const char *directory, const char *filter,
                      IterCallback callback, void *context);

// Do whatever work is necessary to be able to work with the system's clock
// functionality
void time_init(void);

TimeT time_now(void);

TimeT time_elapsed_now(TimeT from);

TimeT time_elapsed(TimeT start, TimeT end);

void time_append(TimeT *a, TimeT b);

int time_expired(TimeT expires_at);

TimeT time_add(TimeT a, TimeT b);

TimeT time_from_secs(int seconds);

int time_gte(TimeT a, TimeT b);

uint64_t time_to_millisecs(TimeT timepoint);
uint64_t time_to_microsecs(TimeT timepoint);
uint64_t time_to_nanosecs(TimeT timepoint);

/* #ifdef ENABLE_DEBUG_TIMERS */
/* #define BeginScopedTimer(name) \ */
/*   static const char *timer_name_##name = (#name); \ */
/*   TimeT start_##name = time_now(); \ */
/*   TimeT end_##name = {}; \ */
/*   TimeT elapsed_##name = {}; */

/* #define PrintScopedTimer(name) \ */
/*   LOG_INFO("Timer '%s': %ld", timer_name_##name, \ */
/*            time_to_microsecs(elapsed_##name)); */

#ifdef ENABLE_DEBUG_TIMERS
#include "statistics.h"

#define DeclareScopedTimerSeries(name)                                         \
  static CircularBuffer *timer_name_##name = statistics_reserve_series(#name);

#define BeginScopedTimerSeries(name)                                           \
  TimeT start_##name = time_now();                                             \
  TimeT end_##name = {};                                                       \
  TimeT elapsed_##name = {};

#define AppendScopedTimerSeries(name)                                          \
  end_##name = time_now();                                                     \
  elapsed_##name = time_elapsed(start_##name, end_##name);                     \
  cbuf_push(timer_name_##name, time_to_microsecs(elapsed_##name));

#define DeclareScopedTimer(name)                                               \
  static Duration *timer_name_##name = statistics_reserve_duration(#name);

#define BeginScopedTimer(name)                                                 \
  TimeT start_##name = time_now();                                             \
  TimeT end_##name = {};                                                       \
  TimeT elapsed_##name = {};

#define AppendScopedTimer(name)                                                \
  end_##name = time_now();                                                     \
  elapsed_##name = time_elapsed(start_##name, end_##name);

#define PrintScopedTimer(name)                                                 \
  timer_name_##name->value = time_to_microsecs(elapsed_##name);

#define FuncTimer(name, func)                                                  \
  DeclareScopedTimer(_##name);                                                 \
  BeginScopedTimer(_##name);                                                   \
  func;                                                                        \
  AppendScopedTimer(_##name);                                                  \
  PrintScopedTimer(_##name);

#else // !ENABLE_DEBUG_TIMERS
#define BeginScopedTimer(name)
#define AppendScopedTimer(name)
#define PrintScopedTimer(name)
#define FuncTimer(name, func)
#endif // ENABLE_DEBUG_TIMERS

// TODO: _mm_sfence() and _mm_lfence() are actual instructions
//       while _Read-/WriteBarrier are just compiler hints. On x64
//       the fences may not be necessary, which lets us not issue
//       those instructions when not required by the architecture.
// TODO: Non-MSVC support here. Which include? Probably not intrin.h?

#ifdef _WIN32
typedef HANDLE SemaphoreHandle;

#define CompletePastWritesBeforeFutureWrites()                                 \
  _WriteBarrier();                                                             \
  _mm_sfence()

#define CompletePastReadsBeforeFutureReads()                                   \
  _ReadBarrier();                                                              \
  _mm_lfence()

#define MakeSemaphore(worker_thread_count)                                     \
  CreateSemaphoreEx(0, 0, (worker_thread_count), "WorkerThreadsSemaphore", 0,  \
                    SEMAPHORE_ALL_ACCESS)

#define WaitForSemaphore(handle)                                               \
  WaitForSingleObjectEx((handle), INFINITE, FALSE)

#define SignalSemaphore(handle, count) ReleaseSemaphore((handle), (count), NULL)

#else
#error "Not implemented for non-Win32 || non-MSVC"
typedef WhateverLinuxUses SemaphoreHandle
#endif

#endif // OS_H
