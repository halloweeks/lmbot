#ifndef LOG_H
#define LOG_H

#include <stdio.h>

/* Simple logging macros */
#define LOGE(fmt, ...) log_error(fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) log_info(fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) log_warn(fmt, ##__VA_ARGS__)

/* Function declarations */
void log_error(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_warn(const char *fmt, ...);

#endif