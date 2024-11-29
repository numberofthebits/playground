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
