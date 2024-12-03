#ifndef _LOG_H
#define _LOG_H


int log_init(const char* file_path);

void log_destroy();

void log_msg(const char* level, const char* file, int line, const char* format,  ...);

#define LOG_INFO(...) \
    log_msg("Info", __FILE__, __LINE__, __FUNCTION__,  __VA_ARGS__);

#define LOG_WARN(...) \
    log_msg("Warning", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);

#define LOG_ERROR(...) \
    log_msg("Error", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);

#define LOG_EXIT(...)                                                    \
    log_msg("Exit", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);      \
    exit(-1); 

#define LOG_GL_HIGH(...)                        \
    log_msg("GL HIGH", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);

#define LOG_GL_MEDIUM(...)                      \
    log_msg("GL MEDIUM", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);

#define LOG_GL_LOW(...)                         \
    log_msg("GL LOW", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);

#define LOG_GL_NOTIFY(...)                \
    log_msg("GL NOTIFICATION", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);
    

#endif _LOG_H
