#include <core/os.h>

#define NOMIXMAX
#define WIN32_LEAN_AND_MEAN
#define _WIN32_LEAN_AND_MEAN
#define TICK_TO_MICROSECS 1000000
#define MICROSECS_TO_TICKS 1 / 1000000
#define TICKS_PER_SECOND 1000
#define SECS_TO_TICKS    1000000000
#define TICKS_TO_SECS 1.0f / (float)(SECS_TO_TICKS)
#define PATH_BUF_MAX 1024

#include <Windows.h>
#include <fileapi.h>
#include <minwinbase.h>
#include <shlwapi.h>
#include <profileapi.h>
#include <stdint.h>
#include "log.h"

static LARGE_INTEGER performance_counter_frequency;

/* // Does not include null terminator */
            /* DWORD path_len = GetFinalPathNameByHandleA(file_handle, path_buf, PATH_BUF_MAX, FILE_NAME_NORMALIZED); */
            
/* HANDLE file_handle = CreateFileA(find_file_data.cFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL); */

            /* if (file_handle == INVALID_HANDLE_VALUE) { */
            /*     FindClose(find_handle); */
            /*     callback("", "", "", context->user_data_instance); */
            /*     break; */
            /* } */


// FindFirstFile just stops if we don't add a path separator at the end,
// so we know the user provided us with that here
void make_file_path(const char* dir, const char* file_name, char* path_buf) {
    strcat(path_buf, dir);
    strcat(path_buf, file_name);
}


void file_system_list(const char* directory, const char* filter, IterCallback callback, void* context) {
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
        for(;;) {
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
                if(GetLastError() != ERROR_NO_MORE_FILES) {
                    LOG_ERROR("Failed to find '%s", search_pattern);
                }
                FindClose(find_handle);
                break;
            }
        }
    }
}



void timers_init() {
    QueryPerformanceFrequency(&performance_counter_frequency);
}

TimeT time_now() {
    LARGE_INTEGER now;             
    QueryPerformanceCounter(&now);

    return now.QuadPart;
}

TimeT time_elapsed_now(TimeT from) {
    TimeT now = time_now();
    TimeT elapsed = now - from;
    return elapsed;
}

int time_expired(TimeT created, TimeT expires_at) {
    TimeT now = time_now();
    if (now >= expires_at) {
        return 1;
    }

    return 0;
}

TimeT time_to_secs(TimeT what_unit_is_this) {
    return (TimeT)((float)(what_unit_is_this * TICKS_TO_SECS));
}

TimeT time_from_secs(TimeT seconds) {
    return seconds * performance_counter_frequency.QuadPart;
}

