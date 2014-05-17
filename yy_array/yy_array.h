//
//  yy_array.h
//  YYMidiBase
//
//  Created by ibireme on 14-2-14.
//  Copyright (c) 2014 ibireme. All rights reserved.
//

#ifndef YYMidiBase_yy_array_h
#define YYMidiBase_yy_array_h


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "yy_base.h"

/// Prototype of a callback function that may be applied to every value in an array.
typedef void (*yy_array_foreach_func)(long index, const void *value, void *context);

/// Prototype of a callback function used to retain a value being added to an array.
typedef void *(*yy_array_retain_callback)(const void *value);

/// Prototype of a callback function used to release a value before it's removed from an array.
typedef void (*yy_array_release_callback)(const void *value);

/// Prototype of a callback function used to determine if two values in an array are equal.
typedef bool (*yy_array_equal_callback)(const void *value1, const void *value2);


typedef struct _yy_array_callback {
    yy_array_retain_callback retain;    ///< called after add object
    yy_array_release_callback release;  ///< called before remove object
    yy_array_equal_callback equal;      ///< called for determine equal
} yy_array_callback_t;



/// Default callback for C string (strdup/free/strcmp)
extern yy_array_callback_t yy_array_string_callback;

/// Default callback for yy object (yy_retain/yy_release/==)
extern yy_array_callback_t yy_array_object_callback;



/**
 YY Array  (similar to CFMutableArray)
 
 Example:
 yy_array_t *array = yy_array_create_for_string();
 yy_array_append(array, "hello");
 yy_array_append(array, "world");
 yy_release(array);
 
 yy_array_t *array = yy_array_create();
 yy_array_append(array, (void*) 1);
 yy_array_append(array, (void*) 2);
 yy_release(array);
 */
typedef struct _yy_array   yy_array_t;

yy_array_t * yy_array_create();
yy_array_t * yy_array_create_for_string();
yy_array_t * yy_array_create_for_object();
yy_array_t * yy_array_create_with_options(long capacity, const yy_array_callback_t *callback);
yy_array_t * yy_array_create_copy(yy_array_t *array);

const void * yy_array_get(yy_array_t *array, long index);
const void * yy_array_get_last(yy_array_t *array, long index);
long yy_array_count(yy_array_t *array);
bool yy_array_get_range(yy_array_t *array, yy_range range, const void **values);
bool yy_array_replace_range(yy_array_t *array, yy_range range, const void **new_values, long new_count);
bool yy_array_append(yy_array_t *array, const void *value);
bool yy_array_prepend(yy_array_t *array, const void *value);
bool yy_array_set(yy_array_t *array, long index, const void *value);
bool yy_array_insert(yy_array_t *array, long index, const void *value);
bool yy_array_remove(yy_array_t *array, long index);
bool yy_array_exchange(yy_array_t *array, long index1, long index2);
bool yy_array_clear(yy_array_t *array);
bool yy_array_contains(yy_array_t *array, const void *value);
long yy_array_get_first_index(yy_array_t *array, yy_range range, const void *value);
long yy_array_get_last_index(yy_array_t *array, yy_range range, const void *value);
bool yy_array_sort(yy_array_t *array, yy_comparator_func cmp, void *context);
bool yy_array_sort_range(yy_array_t *array, yy_range range, yy_comparator_func cmp, void *context);
bool yy_array_foreach(yy_array_t *array, yy_array_foreach_func func, void *context);
bool yy_array_foreach_range(yy_array_t *array, yy_range range, yy_array_foreach_func func, void *context);

#endif
