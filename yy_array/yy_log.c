//
//  yy_log.c
//  YYMidiBase
//
//  Created by ibireme on 14-2-15.
//  Copyright (c) 2014 ibireme. All rights reserved.
//

#include "yy_log.h"

#include <stdarg.h>

yy_LOG_LEVEL yy_log_level = yy_DEFAULT_LOG_LEVEL;

static void _default_yy_log_func(yy_LOG_LEVEL level, char *fmt, va_list args);
void (*yy_log_func)(yy_LOG_LEVEL level, char *fmt, va_list args) = _default_yy_log_func;


static void _default_yy_log_func(yy_LOG_LEVEL level, char *fmt, va_list args) {
    if (level < yy_log_level || level >= yy_LOG_LEVEL_OFF) return;
    switch (level) {
        case yy_LOG_LEVEL_ALL: printf("[yy_LOG] "); break;
        case yy_LOG_LEVEL_INFO: printf("[yy_INFO] "); break;
        case yy_LOG_LEVEL_WARN: printf("[yy_WARN] "); break;
        case yy_LOG_LEVEL_ERROR: printf("[yy_ERROR] "); break;
        default:  break;
    }
    vprintf(fmt, args);
    printf("\n");
}

#define yy_DO_LOG(level) do { \
    va_list args; \
    va_start(args, fmt); \
    yy_log_func((level), fmt, args); \
    va_end(args); \
} while (0)

void yy_log(yy_LOG_LEVEL level, char *fmt, ...) {
    yy_DO_LOG(level);
}

void yy_log_info(char *fmt, ...) {
    yy_DO_LOG(yy_LOG_LEVEL_INFO);
}

void yy_log_warn(char *fmt, ...) {
    yy_DO_LOG(yy_LOG_LEVEL_WARN);
}

void yy_log_error(char *fmt, ...) {
    yy_DO_LOG(yy_LOG_LEVEL_ERROR);
}
