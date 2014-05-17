//
//  yy_log.h
//  YYMidiBase
//
//  Created by ibireme on 14-2-15.
//  Copyright (c) 2014 ibireme. All rights reserved.
//

#ifndef YYMidiBase_yy_log_h
#define YYMidiBase_yy_log_h

#include <stdarg.h>
#include <stdio.h>

typedef enum {
    yy_LOG_LEVEL_ALL,
    yy_LOG_LEVEL_INFO,
    yy_LOG_LEVEL_WARN,
    yy_LOG_LEVEL_ERROR,
    yy_LOG_LEVEL_OFF
} yy_LOG_LEVEL;


#ifndef yy_DEFAULT_LOG_LEVEL
  #ifdef DEBUG
    #define yy_DEFAULT_LOG_LEVEL yy_LOG_LEVEL_INFO
  #else
    #define yy_DEFAULT_LOG_LEVEL yy_LOG_LEVEL_ERROR
  #endif
#endif

extern yy_LOG_LEVEL yy_log_level;
extern void (*yy_log_func)(yy_LOG_LEVEL level, char *fmt, va_list args);

void yy_log(yy_LOG_LEVEL level, char *fmt, ...);
void yy_log_info(char *fmt, ...);
void yy_log_warn(char *fmt, ...);
void yy_log_error(char *fmt, ...);


#endif
