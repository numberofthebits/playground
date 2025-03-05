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

uint64_t time_to_nanosecs(TimeT timepoint);

TimeT time_from_secs(int seconds);

uint64_t time_to_microsecs(TimeT timepoint);

#ifdef ENABLE_DEBUG_TIMERS
#define BeginScopedTimer(name)                                                 \
  static const char *timer_name_##name = (#name);                              \
  TimeT start_##name = time_now();                                             \
  TimeT end_##name = {0};                                                      \
  TimeT elapsed_##name = {0};

#define AppendScopedTimer(name)                                                \
  end_##name = time_now();                                                     \
  elapsed_##name = time_elapsed(start_##name, end_##name);

#define PrintScopedTimer(name)                                                 \
  LOG_INFO("Timer '%s': %ld", timer_name_##name,                               \
           time_to_microsecs(elapsed_##name));

#else // !ENABLE_DEBUG_TIMERS
#define BeginScopedTimer(name)
#define AppendScopedTimer(name)
#define PrintScopedTimer(name)
#endif // ENABLE_DEBUG_TIMERS

#endif // OS_H
