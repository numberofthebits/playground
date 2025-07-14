#include "log.h"

#include "os.h"

#include <stdarg.h>
#include <stdio.h>
#include <threads.h>
#include <time.h>

unsigned long long time_logging;

static thread_local char log_buffer[4096 * 4];
static FILE *fp;

// TODO: Remove mutex?
// NOTE: C11 standard guarantees thread safety for printf. Mutex not
//       required for that then.
// NOTE: Since we prep the buffer per thread (thread_local log_buffer)
//       this should be safe?
/* static mtx_t log_print_mutex; */

typedef void (*LogFunc)(int);

static LogFunc log_impl;

static void log_to_console(int len) {
  //  mtx_lock(&log_print_mutex);

  (void)len;
  printf("%s", log_buffer);

  // mtx_unlock(&log_print_mutex);
}

static void log_to_file(int len) {
  //  mtx_lock(&log_print_mutex);

  fwrite(log_buffer, len, 1, fp);
  fflush(fp);

  //  mtx_unlock(&log_print_mutex);
}

int log_init(const char *file_path) {
  time_init();

  /* if (mtx_init(&log_print_mutex, mtx_plain) != thrd_success) { */
  /*   return 0; */
  /* } */

  if (file_path) {
    fp = fopen("log.txt", "w+b");

    if (!fp) {
      printf("Failed to open log file '%s'\n", file_path);
      return 0;
    }

    log_impl = &log_to_file;
    return 1;
  }

  log_impl = &log_to_console;

  return 1;
}

void log_destroy(void) {
  if (fp) {
    fclose(fp);
  }
}

int make_time_str(char *buf, int buf_size) {
  time_t now;
  struct tm *time_info;
  time(&now);
  time_info = localtime(&now);

  return (int)strftime(buf, buf_size, "[%H:%M:%S]", time_info);
}

void log_msg(const char *level, const char *file, int line,
             const char *func_name, const char *format, ...) {
  TimeT t0 = time_now();
  int offset = make_time_str(log_buffer, sizeof(log_buffer));

  offset += sprintf(log_buffer + offset, "[%s][%s:%d:%s] ", level, file, line,
                    func_name);

  va_list args;
  va_start(args, format);

  offset += vsprintf(log_buffer + offset, format, args);
  va_end(args);

  log_buffer[offset++] = '\n';

  log_impl(offset);
  TimeT t1 = time_now();
  time_logging += time_to_microsecs(time_elapsed(t0, t1));
}
