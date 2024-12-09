#include "util.h"

#include <stdio.h>
#include <stdlib.h>

LARGE_INTEGER performance_counter_frequency;

unsigned char* file_read_all(const char* file_path) {
    FILE* fp = fopen(file_path, "r+b");
    if(!fp) {
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    LOG_INFO("File size %d bytes", size);
    unsigned char* data = malloc(size + 1);
    data[size] = 0;

    fread(data, size, 1, fp);
    fclose(fp);

    return data;
}

void timers_init() {
    QueryPerformanceFrequency(&performance_counter_frequency);
}

uint64_t time_now() {
    LARGE_INTEGER now;             
    QueryPerformanceCounter(&now);

    return now.QuadPart;
}

uint64_t time_elapsed(uint64_t from) {
    uint64_t now = time_now();
    uint64_t elapsed = now - from;
    return elapsed;
}

int time_expired(uint64_t created, uint64_t expires) {

    uint64_t elapsed = time_elapsed(created);
    if (elapsed >= expires) {
        return 1;
    }

    return 0;
}

uint64_t time_to_secs(uint64_t what_unit_is_this) {
    return (uint64_t)((float)(what_unit_is_this * TICKS_TO_SECS));
}

uint64_t time_from_secs(uint64_t what_unit_is_this) {
    return what_unit_is_this * SECS_TO_TICKS;
}
