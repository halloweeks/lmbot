#include "log.h"
#include <stdarg.h>

static void log_base(const char *prefix, const char *fmt, va_list args)
{
    fprintf(stderr, "%s", prefix);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

void log_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_base("[ERROR] ", fmt, args);
    va_end(args);
}

void log_warn(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_base("[WARN ] ", fmt, args);
    va_end(args);
}

void log_info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_base("[INFO ] ", fmt, args);
    va_end(args);
}