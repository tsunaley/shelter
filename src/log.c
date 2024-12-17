#include "log.h"
#include <stdarg.h>

/*
 * 函数 log_level_to_string() 用于将日志级别转换为对应的字符串表示
 * 参数:
 *   - LogLevel level: 日志级别
 * 返回值:
 *   - const char*: 对应日志级别的字符串表示
 */
static const char* log_level_to_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

/*
 * 函数 log_message() 用于打印日志信息，并在日志级别为错误时退出程序
 * 参数:
 *   - LogLevel level: 日志级别
 *   - const char *file: 源文件名称，用于标记日志信息来源
 *   - int line: 源文件中的行号，用于标记日志信息来源
 *   - const char *fmt: 格式化字符串，用于打印日志消息
 *   - ...: 可变参数，用于格式化输出日志信息
 * 返回值:
 *   - void: 无返回值
 */
void log_message(LogLevel level, const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    time_t now;
    time(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(stderr, "[%s] [%s] [%s:%d] ", time_str, log_level_to_string(level), file, line);
    vfprintf(stderr, fmt, args);
    if (level == LOG_LEVEL_ERROR) {
        fprintf(stderr, ": %s (errno: %d)", strerror(errno), errno);
    }
    fprintf(stderr, "\n");

    va_end(args);

    if (level == LOG_LEVEL_ERROR) {
        exit(EXIT_FAILURE);
    }
}
