//
//  yy_map.h
//  YYMidiBase
//
//  Created by ibireme on 14-2-20.
//  Copyright (c) 2014 ibireme. All rights reserved.
//

#ifndef YYMidiBase_yy_map_h
#define YYMidiBase_yy_map_h

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "yy_base.h"
#include "yy_array.h"

/// Prototype of a callback function that may be applied to every key-value pair in a map.
typedef void (*yy_map_foreach_func)(const void *key, const void *value, void *context);

/// Prototype of a callback function used to retain a key or value being added to a map.
typedef void *(*yy_map_retain_callback)(const void *value);

/// Prototype of a callback function used to release a key or value before it's removed from a map.
typedef void (*yy_map_release_callback)(const void *value);

/// Prototype of a callback function used to determine if two key or value in a map are equal.
typedef bool (*yy_map_equal_callback)(const void *value1, const void *value2);

/// Prototype of a callback function invoked to compute a hash code for a key. Hash codes are used when key-value pairs are accessed, added, or removed from a collection.
typedef unsigned long (*yy_map_hash_callback)(const void *value);


/// This structure contains the callbacks used to retain, release, hash and compare the keys in a dictionary.
typedef struct _yy_map_key_callback {
    yy_map_retain_callback retain;
    yy_map_release_callback release;
    yy_map_equal_callback equal;
    yy_map_hash_callback hash;
} yy_map_key_callback_t;

/// This structure contains the callbacks used to retain, release and compare the values in a dictionary.
typedef struct _yy_map_value_callback {
    yy_map_retain_callback retain;
    yy_map_release_callback release;
    yy_map_equal_callback equal;
} yy_map_value_callback_t;



/// Default c string key callback.
extern yy_map_key_callback_t yy_map_string_key_callback;

/// Default yy object key callback.
extern yy_map_key_callback_t yy_map_object_key_callback;

/// Default c string value callback.
extern yy_map_value_callback_t yy_map_string_value_callback;

/// Defaut yy object value callback.
extern yy_map_value_callback_t yy_map_object_value_callback;


/**
 YY Map  (Similar to CFMutableDictionary)
 
 Example:
 
 yy_map_t *map = yy_map_create_with_options(0, yy_map_string_key_callback, yy_map_string_value_callback);
 yy_map_set("Name", "Steve");
 yy_map_set("Age", "24");
 char *name = yy_map_get(map, "Name");
 yy_release(map);
 */
typedef struct _yy_map yy_map_t;

yy_map_t *yy_map_create();
yy_map_t *yy_map_create_with_options(long capacity,
                                     const yy_map_key_callback_t *key_callback,
                                     const yy_map_value_callback_t *value_callback);

long yy_map_count(yy_map_t *map);
bool yy_map_contains_key(yy_map_t *map, const void *key);
bool yy_map_contains_value(yy_map_t *map,const void *value);
const void *yy_map_get(yy_map_t *map, const void *key);
bool yy_map_set(yy_map_t *map, const void *key, const void *value);
bool yy_map_set_all(yy_map_t *map, yy_map_t *add);
bool yy_map_remove(yy_map_t *map, const void *key);
bool yy_map_clear(yy_map_t *map);
bool yy_map_get_all_keys(yy_map_t *map, const void **keys);
bool yy_map_foreach(yy_map_t *map, yy_map_foreach_func func, void *context);
yy_array_t *yy_map_create_key_array(yy_map_t *map);

#endif
