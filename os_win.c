#include "os.h"

#include "log.h"

#define _WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <fileapi.h>
#include <minwinbase.h>
#include <shlwapi.h>

#define PATH_BUF_MAX 1024

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
