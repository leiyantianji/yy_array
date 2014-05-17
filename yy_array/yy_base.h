//
//  yy_def.h
//  YYMidiBase
//
//  Created by ibireme on 14-2-13.
//  Copyright (c) 2014 ibireme. All rights reserved.
//

#ifndef YYMidiBase_yy_def_h
#define YYMidiBase_yy_def_h

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>


/******************************* define ***************************************/

#if !defined(yy_inline)
    #if defined(__GNUC__) && (__GNUC__ == 4) && !defined(DEBUG)
        #define yy_inline static __inline__ __attribute__((always_inline))
    #elif defined(__GNUC__)
        #define yy_inline static __inline__
    #elif defined(__cplusplus)
        #define yy_inline static inline
    #elif defined(_MSC_VER)
        #define yy_inline static __inline
    #elif TARGET_OS_WIN32
        #define yy_inline static __inline__
    #elif defined(WIN32)
        #define yy_inline static __inline__
    #endif
#endif


/******************************* range and order ******************************/

typedef struct {
    long location;
    long length;
} yy_range;


yy_inline yy_range yy_range_make(long location, long length) {
    yy_range range;
    range.location = location;
    range.length = length;
    return range;
}

enum {
    YY_NOT_FOUND = -1
};

typedef enum {
    YY_ORDER_ASC = -1,
    YY_ORDER_EQUAL = 0,
    YY_ORDER_DESC = 1
} yy_order;

typedef yy_order (*yy_comparator_func)(const void *value1, const void *value2, void *context);



/******************************* object (Similar to CoreFounation API) *****************************/

/**
 Retains a YY object. (ref-count +1)
 */
const void *yy_retain(void *object);

/**
 Release a YY object. (ref-count +1)
 */
void yy_release(void *object);

/**
 Get retain count
 */
long yy_retain_count(void *object);


#endif
