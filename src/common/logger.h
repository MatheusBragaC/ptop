#ifndef LOGGER_H
#define LOGGER_H

typedef enum {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;

void log_init();
void log_cleanup();

// Main logging function
void log_msg(LogLevel level, const char *fmt, ...);

// Helper for errno logging (replaces perror)
void log_error_errno(const char *msg);

#endif
