#ifndef _MC_LOG_H_
#define _MC_LOG_H_

typedef enum {
    LOG_EMERG = 0, /* system in unusable */
    LOG_ALERT,     /* system in unusable */
    LOG_ERR,       /* system error       */
    LOG_WARN,      /* warning conditions */
    LOG_INFO,      /* informational      */
    LOG_DEBUG,     /* debug messages     */
} LOG_LEVEL;

/*
 * log_stderr   - log to stderr
 * loga         - log always
 * loga_hexdump - log hexdump always
 * log_error    - error log messages
 * log_warn     - warning log messages
 * log_panic    - log messages followed by a panic
 * ...
 * log_debug    - debug log messages based on a log level
 * log_hexdump  - hexadump -C of a log buffer
 */

#define log_stderr(...)                               \
    do {                                              \
        _log_stderr(__FILE__, __LINE__, __VA_ARGS__); \
    } while (0)

#define log_hexdump(_level, _data, _datalen, ...)                  \
    do {                                                           \
        if (log_loggable(_level) != 0) {                           \
            _log_core(_level, __FILE__, __LINE__, 0, __VA_ARGS__); \
            _log_hexdump((char*)(_data), (int)(_datalen));         \
        }                                                          \
    } while (0)

#define log_debug(...)                                                \
    do {                                                              \
        if (log_loggable(LOG_DEBUG) != 0) {                           \
            _log_core(LOG_DEBUG, __FILE__, __LINE__, 0, __VA_ARGS__); \
        }                                                             \
    } while (0)

#define log_debug_x(_file, _line, ...)                          \
    do {                                                        \
        if (log_loggable(LOG_DEBUG) != 0) {                     \
            _log_core(LOG_DEBUG, _file, _line, 0, __VA_ARGS__); \
        }                                                       \
    } while (0)

#define log_info(...)                                                \
    do {                                                             \
        if (log_loggable(LOG_INFO) != 0) {                           \
            _log_core(LOG_INFO, __FILE__, __LINE__, 0, __VA_ARGS__); \
        }                                                            \
    } while (0)

#define log_info_x(_file, _line, ...)                          \
    do {                                                       \
        if (log_loggable(LOG_INFO) != 0) {                     \
            _log_core(LOG_INFO, _file, _line, 0, __VA_ARGS__); \
        }                                                      \
    } while (0)

#define log_warn(...)                                                \
    do {                                                             \
        if (log_loggable(LOG_WARN) != 0) {                           \
            _log_core(LOG_WARN, __FILE__, __LINE__, 0, __VA_ARGS__); \
        }                                                            \
    } while (0)

#define log_warn_x(_file, _line, ...)                          \
    do {                                                       \
        if (log_loggable(LOG_WARN) != 0) {                     \
            _log_core(LOG_WARN, _file, _line, 0, __VA_ARGS__); \
        }                                                      \
    } while (0)

#define log_error(...)                                              \
    do {                                                            \
        if (log_loggable(LOG_ERR) != 0) {                           \
            _log_core(LOG_ERR, __FILE__, __LINE__, 0, __VA_ARGS__); \
        }                                                           \
    } while (0)

#define log_error_x(_file, _line, ...)                        \
    do {                                                      \
        if (log_loggable(LOG_ERR) != 0) {                     \
            _log_core(LOG_ERR, _file, _line, 0, __VA_ARGS__); \
        }                                                     \
    } while (0)

#define log_panic(...)                                                \
    do {                                                              \
        if (log_loggable(LOG_EMERG) != 0) {                           \
            _log_core(LOG_EMERG, __FILE__, __LINE__, 1, __VA_ARGS__); \
        }                                                             \
    } while (0)

#define log_panic_x(_file, _line, ...)                          \
    do {                                                        \
        if (log_loggable(LOG_EMERG) != 0) {                     \
            _log_core(LOG_EMERG, _file, _line, 1, __VA_ARGS__); \
        }                                                       \
    } while (0)

int log_init(LOG_LEVEL level, char* filename);

void log_release(void);

void log_level_up(void);

void log_level_down(void);

void log_level_set(LOG_LEVEL level);

void log_reopen(void);

int log_loggable(LOG_LEVEL level);

void _log_core(LOG_LEVEL log_level, const char* file, int line, int panic, const char* fmt, ...);

void _log_stderr(const char* file, int line, const char* fmt, ...);

void _log_hexdump(char* data, int datalen);

#endif
