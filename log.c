#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

static char log_buffer[4096*4];
static FILE* fp;

typedef void(*LogFunc)();

static LogFunc log_impl;

static void log_to_console(int len) {
    (void)len;
    printf("%s", log_buffer);
}

static void log_to_file(int len) {
    fwrite(log_buffer, len, 1, fp);
    fflush(fp);
}

int log_init(const char* file_path) {
    if(file_path) {
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

void log_destroy() {
    if (fp) {
        fflush(fp);
        fclose(fp);
    }
}

int make_time_str(char* buf, int buf_size) {
    time_t now;
    struct tm* time_info;
    time(&now);
    time_info = localtime(&now);

    return (int)strftime(buf, buf_size, "[%H:%M:%S]", time_info);
}

void log_msg(const char* level, const char* file, int line, const char* func_name, const char* format,  ...) {
    int offset = make_time_str(log_buffer, sizeof(log_buffer));
    
    offset += sprintf(log_buffer + offset, "[%s][%s:%d:%s] ", level, file, line, func_name);

    va_list args;
    va_start(args, format);
    
    offset += vsprintf(log_buffer + offset, format, args);
    va_end(args);

    offset += sprintf(log_buffer + offset, "\n");

    log_impl(offset);
}
