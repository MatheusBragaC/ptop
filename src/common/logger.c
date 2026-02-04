#include "logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <time.h>

static FILE *log_file = NULL;

void log_init()
{
    // For now, we log to a file because stdout is used by the UI
    log_file = fopen("ptop.log", "w");
}

void log_cleanup()
{
    if (log_file)
    {
        fclose(log_file);
        log_file = NULL;
    }
}

void log_msg(LogLevel level, const char *fmt, ...)
{
    if (!log_file) return;

    // Timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", t);

    // Level String
    const char *level_str = "INFO";
    if (level == LOG_WARN) level_str = "WARN";
    else if (level == LOG_ERROR) level_str = "ERROR";
    else if (level == LOG_FATAL) level_str = "FATAL";

    fprintf(log_file, "[%s] [%s] ", time_str, level_str);

    va_list args;
    va_start(args, fmt);
    vfprintf(log_file, fmt, args);
    va_end(args);

    fprintf(log_file, "\n");
    fflush(log_file);
}

void log_error_errno(const char *msg)
{
    log_msg(LOG_ERROR, "%s: %s", msg, strerror(errno));
}
