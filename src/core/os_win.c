#include "os.h"

#define WIN32_LEAN_AND_MEAN
#define TICK_TO_MILLISECS 1000
#define TICK_TO_MICROSECS 1000000
#define TICK_TO_NANOSECS 1000000000
#define MICROSECS_TO_TICKS 1 / 1000000
#define TICKS_PER_SECOND 1000
#define SECS_TO_TICKS 1000000000
#define TICKS_TO_SECS 1.0f / (float)(SECS_TO_TICKS)
#define PATH_BUF_MAX 1024

#include "log.h"
#include <Windows.h>
#include <fileapi.h>
#include <minwinbase.h>
#include <profileapi.h>
#include <shlwapi.h>
#include <stdint.h>

static LARGE_INTEGER performance_counter_frequency;

/* // Does not include null terminator */
/* DWORD path_len = GetFinalPathNameByHandleA(file_handle, path_buf,
 * PATH_BUF_MAX, FILE_NAME_NORMALIZED); */

/* HANDLE file_handle = CreateFileA(find_file_data.cFileName, GENERIC_READ,
 * FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL); */

/* if (file_handle == INVALID_HANDLE_VALUE) { */
/*     FindClose(find_handle); */
/*     callback("", "", "", context->user_data_instance); */
/*     break; */
/* } */

// FindFirstFile just stops if we don't add a path separator at the end,
// so we know the user provided us with that here
void make_file_path(const char *dir, const char *file_name, char *path_buf) {
  strcat(path_buf, dir);
  strcat(path_buf, file_name);
}

void file_system_list(const char *directory, const char *filter,
                      IterCallback callback, void *context) {
  char search_pattern[1024] = {0};
  strcat(search_pattern, directory);
  if (filter) {
    strcat(search_pattern, filter);
  }
  WIN32_FIND_DATA find_file_data;
  HANDLE find_handle = FindFirstFile(search_pattern, &find_file_data);

  if (find_handle == INVALID_HANDLE_VALUE) {
    FindClose(find_handle);
    LOG_ERROR("Failed to find '%s", search_pattern);
    return;
  } else {
    for (;;) {
      FileSystemListResult result = {0};
      char path_buf[PATH_BUF_MAX] = {0};
      make_file_path(directory, find_file_data.cFileName, path_buf);

      result.file_path = path_buf;
      result.extension = PathFindExtensionA(path_buf);
      result.file_name = find_file_data.cFileName;

      int callback_result = callback(&result, context);

      // Client wants to stop
      if (callback_result != FILE_CALLBACK_RESULT_CONTINUE) {
        break;
      }

      if (!FindNextFile(find_handle, &find_file_data)) {
        if (GetLastError() != ERROR_NO_MORE_FILES) {
          LOG_ERROR("Failed to find '%s", search_pattern);
        }
        FindClose(find_handle);
        break;
      }
    }
  }
}

void time_init() { QueryPerformanceFrequency(&performance_counter_frequency); }

TimeT time_now() {
  LARGE_INTEGER now;
  QueryPerformanceCounter(&now);

  return now;
}

TimeT time_elapsed(TimeT start, TimeT end) {
  TimeT result = {0};
  result.QuadPart = end.QuadPart - start.QuadPart;

  return result;
}

void time_append(TimeT *a, TimeT b) { a->QuadPart += b.QuadPart; }

TimeT time_elapsed_now(TimeT from) {
  TimeT now = time_now();
  TimeT elapsed;
  elapsed.QuadPart = now.QuadPart - from.QuadPart;
  return elapsed;
}

int time_expired(TimeT expires_at) {
  TimeT now = time_now();
  if (now.QuadPart >= expires_at.QuadPart) {
    return 1;
  }

  return 0;
}

TimeT time_to_secs(TimeT timepoint) {
  timepoint.QuadPart = (LONGLONG)((float)timepoint.QuadPart * TICKS_TO_SECS);
  return timepoint;
}

TimeT time_from_secs(int seconds) {
  TimeT result = {0};
  result.QuadPart = seconds * performance_counter_frequency.QuadPart;

  return result;
}

uint64_t time_to_millisecs(TimeT timepoint) {
  return timepoint.QuadPart * TICK_TO_MILLISECS /
         performance_counter_frequency.QuadPart;
}

uint64_t time_to_microsecs(TimeT timepoint) {
  return timepoint.QuadPart * TICK_TO_MICROSECS /
         performance_counter_frequency.QuadPart;
}

uint64_t time_to_nanosecs(TimeT timepoint) {
  return timepoint.QuadPart * TICK_TO_NANOSECS /
         performance_counter_frequency.QuadPart;
}

TimeT time_add(TimeT a, TimeT b) {
  TimeT result = {0};
  result.QuadPart = a.QuadPart + b.QuadPart;
  return result;
}
