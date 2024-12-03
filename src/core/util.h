#ifndef _UTIL_H
#define _UTIL_H

#include "log.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <profileapi.h>

#define TICKS_PER_SECOND 1000000

extern LARGE_INTEGER performance_counter_frequency;

unsigned char* file_read_all(const char* file_path);

void timers_init();

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

#endif _UTIL_H
