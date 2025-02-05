#include <core/os.h>

#include "log.h"

#include <time.h>
#include <stdlib.h>

#define SECS_TO_NANO 1000000000
#define SECS_TO_MICRO 1000000
#define NANO_TO_MICRO 1000

#include <dirent.h>

static TimeT* clock_resolution;

void file_system_list(const char* directory, const char* filter, IterCallback callback, void* context) {
    (void)directory;
    (void)filter;
    (void)callback;
    (void)context;
    // TODO: Implement function
}

void time_init(void) {

  clock_resolution = malloc(sizeof(TimeT));
  if (clock_getres(CLOCK_MONOTONIC_RAW, clock_resolution) < 0) {
    LOG_ERROR("clock_getres() failed");
  }
}

TimeT time_now(void) {
  TimeT now = {0};
  if (clock_gettime(CLOCK_MONOTONIC_RAW, &now) < 0) {
    LOG_ERROR("Failed to get time");
  }

  return now;
}

TimeT time_elapsed(TimeT start, TimeT end) {
  TimeT result;

  result.tv_sec=  end.tv_sec - start.tv_sec;
  result.tv_nsec = end.tv_nsec - start.tv_nsec;
  
  return result;
}

TimeT time_elapsed_now(TimeT from) {
  TimeT now = time_now();
  return time_elapsed(now, from);
}

void time_append(TimeT* a, TimeT b) {
  a->tv_sec += b.tv_sec;
  a->tv_nsec += b.tv_nsec;
}

int time_expired(TimeT expires_at) {
  TimeT now = time_now();

  if (now.tv_sec < expires_at.tv_sec) return 0;
  else if (now.tv_sec == expires_at.tv_sec &&
	   now.tv_nsec <= expires_at.tv_nsec) return 0;
  
  return 1;
}


uint64_t time_to_nanosecs(TimeT timepoint) {
  return timepoint.tv_sec * SECS_TO_NANO + timepoint.tv_nsec;
}

TimeT time_from_secs(int seconds) {
  TimeT value;
  value.tv_sec = seconds;
  value.tv_nsec = 0;
  return value;
}

uint64_t time_to_microsecs(TimeT timepoint) {
  return timepoint.tv_sec * SECS_TO_MICRO + timepoint.tv_nsec / NANO_TO_MICRO;
}

TimeT time_add(TimeT a, TimeT b) {
  TimeT ret;
  ret.tv_sec = a.tv_sec + b.tv_sec;
  ret.tv_nsec = a.tv_nsec + b.tv_nsec;
  return ret;
}
