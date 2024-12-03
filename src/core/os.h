#ifndef _OS_H
#define _OS_H

#define FILE_CALLBACK_RESULT_CONTINUE 0
#define FILE_CALLBACK_RESULT_STOP 1

typedef struct {
    const char* file_path;
    const char* file_name;
    const char* extension;
} FileSystemListResult;

typedef int(*IterCallback)(const FileSystemListResult* result, void* context);

// Ask file system to list contents of 'directory'. Optionally filter by
// wildcards
void file_system_list(const char* directory, const char* filter, IterCallback callback, void* context);

#endif _OS_H

