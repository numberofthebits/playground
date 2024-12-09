#ifndef UTIL_H
#define UTIL_H

#include "log.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <profileapi.h>

#include <stdint.h>

#define TICKS_PER_SECOND 1000000

#define SECS_TO_TICKS 1000000000
#define TICKS_TO_SECS 1.0f / (float)(SECS_TO_TICKS)


extern LARGE_INTEGER performance_counter_frequency;

unsigned char* file_read_all(const char* file_path);

void timers_init();

uint64_t time_now();

uint64_t time_elapsed(uint64_t from);

int time_expired(uint64_t created, uint64_t expires);

uint64_t time_to_secs(uint64_t what_unit_is_this);

uint64_t time_from_secs(uint64_t what_unit_is_this);

#define BeginScopedTimer(name)                          \
    static const char* timer_name_##name = (#name);     \
    LARGE_INTEGER start_##name;                         \
    QueryPerformanceCounter(&start_##name);             \

#define AppendScopedTimer(name)                 \
    LARGE_INTEGER end_##name;                   \
    QueryPerformanceCounter(&end_##name);       \

#define PrintScopedTimer(name)                                          \
    LARGE_INTEGER elapsed_##name;                                       \
    elapsed_##name.QuadPart = end_##name.QuadPart - start_##name.QuadPart; \
    elapsed_##name.QuadPart *= TICKS_PER_SECOND;                        \
    elapsed_##name.QuadPart /= performance_counter_frequency.QuadPart;  \
    LOG_INFO("Timer '%s': %ld", timer_name_##name, elapsed_##name.QuadPart);   \

#endif UTIL_H
