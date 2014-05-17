//
//  yy_map.c
//  YYMidiBase
//
//  Created by ibireme on 14-2-20.
//  Copyright (c) 2014 ibireme. All rights reserved.
//

#include "yy_map.h"
#include "yy_log.h"
#include "yy_base_private.h"

#include <string.h>
#include <limits.h>


/**
 * Pointer Hash Function.
 * Just return it...
 */
static unsigned long _yy_map_hash_callback_default(const void *key) {
    return (unsigned long)key;
}

/**
 * BKDR Hash Function.
 * Using a strange set of possible seeds: 31 131 1313 13131 etc...
 */
static unsigned long _yy_map_string_hash_callbak(const void *key) {
    unsigned long hash;
    char *str;
    
    hash = 0;
    str = (char *)key;
    while (*str)
        hash = hash * 131 + (*str++); //31 131 1313 13131 etc..
    return hash;
}

static bool _yy_map_string_equal_callback(const void *key1, const void *key2) {
    return strcmp(key1, key2) == 0;
}

yy_map_key_callback_t yy_map_string_key_callback = {
    (yy_map_retain_callback)strdup,
    (yy_map_release_callback)free,
    _yy_map_string_equal_callback,
    _yy_map_string_hash_callbak,
};

yy_map_key_callback_t yy_map_object_key_callback = {
    (yy_map_retain_callback)yy_retain,
    (yy_map_release_callback)yy_release,
    NULL,
    _yy_map_hash_callback_default,
};

yy_map_value_callback_t yy_map_string_value_callback = {
    (yy_map_retain_callback)strdup,
    (yy_map_release_callback)free,
    _yy_map_string_equal_callback,
};

yy_map_value_callback_t yy_map_object_value_callback = {
    (yy_map_retain_callback)yy_retain,
    (yy_map_release_callback)yy_release,
    NULL,
};


typedef struct _yy_map_node   yy_map_node_t;

struct _yy_map_node {
    const void *key;
    const void *value;
    unsigned long hash;
    yy_map_node_t *next;
};

struct _yy_map {
    long node_count;
    long bucket_count;
    yy_map_node_t **buckets;
    yy_map_key_callback_t key_callback;
    yy_map_value_callback_t value_callback;
};



yy_inline yy_map_node_t * _yy_map_get_node(yy_map_t *map, yy_map_node_t **bucket, const void *key) {
    yy_map_node_t *node;
    
    node = *bucket;
    while (node) {
        if (node->key == key
            || (map->key_callback.equal &&  map->key_callback.equal(node->key, key) )) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

yy_inline void _yy_map_resize(yy_map_t *map, long new_bucket_count) {
    yy_map_node_t **new_buckets, *node, *next_node, *new_node;
    long i, new_bucket_index;
    
    new_buckets = calloc(new_bucket_count, sizeof(yy_map_node_t *));
    if (new_buckets == NULL) {
        yy_log_error("yy_array_t:%s() attempt to allocate %ld bytes failed",
                     __func__, new_bucket_count * sizeof(yy_map_node_t *));
        return;
    }
    
    for (i = 0; i < map->bucket_count; i++) {
        node = map->buckets[i];
        while (node) {
            next_node = node->next;
            node->next = NULL;
            new_bucket_index = node->hash % new_bucket_count;
            new_node = new_buckets[new_bucket_index];
            if (!new_node) {
                new_buckets[new_bucket_index] = node;
            } else {
                while (new_node->next) new_node = new_node->next;
                new_node->next = node;
            }
            node = next_node;
        }
    }
    free(map->buckets);
    map->buckets = new_buckets;
    map->bucket_count = new_bucket_count;
}

static void _yy_map_dealloc(yy_map_t *map) {
    yy_map_clear(map);
    free(map->buckets);
    yy_dealloc(map);
}

yy_map_t * yy_map_create() {
    return yy_map_create_with_options(0, NULL, NULL);
}

yy_map_t * yy_map_create_with_options(long                          capacity,
                                      const yy_map_key_callback_t   *key_callback,
                                      const yy_map_value_callback_t *value_callback) {
    yy_map_t *map;
    
    if (capacity < 0) {
        yy_log_error("%s() capacity(%ld) cannot be less than zero",
                     __func__, capacity);
        return NULL;
    }
    if (capacity < 13) capacity = 13; // min buckets count.
    
    /// TODO optimize capacity
    
    map = yy_alloc(yy_map_t, _yy_map_dealloc);
    
    if (map == NULL) {
        yy_log_error("yy_map_t:%s() attempt to allocate %ld bytes failed",
                     __func__, sizeof(yy_map_t));
        return NULL;
    }
    
    map->buckets = calloc(capacity, sizeof(yy_map_node_t *));
    if (map->buckets == NULL) {
        yy_dealloc(map);
        yy_log_error("yy_map_t:%s() attempt to allocate %ld bytes failed",
                     __func__, capacity * sizeof(yy_map_node_t *));
        return NULL;
    }
    
    map->node_count = 0;
    map->bucket_count = capacity;
    if (key_callback) map->key_callback = *key_callback;
    if (value_callback) map->value_callback = *value_callback;
    if (map->key_callback.hash == NULL) {
        map->key_callback.hash = _yy_map_hash_callback_default;
    }
    return map;
}

long yy_map_count(yy_map_t *map) {
    return map->node_count;
}

bool yy_map_contains_key(yy_map_t *map, const void *key) {
    yy_map_node_t **bucket, *node;
    unsigned long bucket_index;
    
    bucket_index = map->key_callback.hash(key) % map->bucket_count;
    bucket = &(map->buckets[bucket_index]);
    node = _yy_map_get_node(map, bucket, key);
    return node != NULL;
}

bool yy_map_contains_value(yy_map_t *map, const void *value) {
    long i;
    yy_map_node_t **bucket, *node;
    
    for (i = 0; i < map->bucket_count; i++) {
        bucket = &map->buckets[i];
        node = *bucket;
        while (node) {
            if (node->value == value
                || (map->value_callback.equal && map->value_callback.equal(node->value, value))) {
                return true;
            }
            node = node->next;
        }
    }
    return false;
}

const void * yy_map_get(yy_map_t *map, const void *key) {
    yy_map_node_t **bucket, *node;
    unsigned long bucket_index;
    
    bucket_index = map->key_callback.hash(key) % map->bucket_count;
    bucket = &(map->buckets[bucket_index]);
    node = _yy_map_get_node(map, bucket, key);
    if (node) return node->value;
    return NULL;
}

bool yy_map_set(yy_map_t *map, const void *key, const void *value) {
    yy_map_node_t **bucket, *node, *cur_node;
    unsigned long bucket_index, hash;
    
    hash = map->key_callback.hash(key);
    bucket_index = hash % map->bucket_count;
    bucket = &(map->buckets[bucket_index]);
    node = _yy_map_get_node(map, bucket, key);
    
    if (node) {
        if (map->value_callback.retain) value = map->value_callback.retain(value);
        if (map->value_callback.release) map->value_callback.release(node->value);
        node->value = value;
    } else {
        node = calloc(1, sizeof(yy_map_node_t));
        if (node == NULL) {
            yy_log_error("yy_map_t:%s() attempt to allocate %ld bytes failed",
                         __func__, sizeof(yy_map_node_t));
            return false;
        }
        
        if (*bucket) {
            cur_node = *bucket;
            while (cur_node->next) cur_node = cur_node->next;
            cur_node->next = node;
        } else {
            *bucket = node;
        }
        
        if (map->key_callback.retain) node->key = map->key_callback.retain(key);
        else node->key = key;
        if (map->value_callback.retain) value = map->value_callback.retain(value);
        node->value = value;
        node->hash = hash;
        map->node_count++;
    }
    
    ///TODO optimize resize!!!
    if (map->node_count > map->bucket_count * 3.0f / 4.0f) {
        _yy_map_resize(map, map->bucket_count * 2 + 1);
    }
    return true;
}

bool yy_map_set_all(yy_map_t *map, yy_map_t *add) {
    long i;
    yy_map_node_t **bucket, *node;
    
    if (add == NULL) return false;
    for (i = 0; i < add->bucket_count; i++) {
        bucket = &add->buckets[i];
        if (!*bucket) continue;
        node = *bucket;
        while (node) {
            yy_map_set(map, node->key, node->value);
            node = node->next;
        }
    }
    return true;
}

bool yy_map_remove(yy_map_t *map, const void *key) {
    yy_map_node_t **bucket, *node, *prev_node;
    unsigned long bucket_index;
    
    bucket_index = map->key_callback.hash(key) % map->bucket_count;
    bucket = &map->buckets[bucket_index];
    
    node = *bucket;
    prev_node = NULL;
    while (node) {
        if (node->key == key
            || (map->key_callback.equal &&  map->key_callback.equal(node->key, key) )) {
            break;
        }
        prev_node = node;
        node = node->next;
    }
    if (node == NULL) return false;
    
    if (map->key_callback.release) map->key_callback.release(node->key);
    if (map->value_callback.release) map->value_callback.release(node->value);
    if (prev_node == NULL) *bucket = node->next;
    else prev_node->next = node->next;
    free(node);
    return true;
}

bool yy_map_clear(yy_map_t *map) {
    long i;
    yy_map_node_t **bucket, *node, *next_node;
    
    for (i = 0; i < map->bucket_count; i++) {
        bucket = &map->buckets[i];
        node = *bucket;
        while (node) {
            if (map->key_callback.release) map->key_callback.release(node->key);
            if (map->value_callback.release) map->value_callback.release(node->value);
            next_node = node->next;
            free(node);
            node = next_node;
        }
        *bucket = NULL;
    }
    return true;
}

bool yy_map_get_all_keys(yy_map_t *map, const void **keys) {
    long i;
    yy_map_node_t **bucket, *node;
    
    if (keys == NULL) return false;
    for (i = 0; i < map->bucket_count; i++) {
        bucket = &map->buckets[i];
        node = *bucket;
        while (node) {
            *keys = node->key;
            keys++;
            node = node->next;
        }
    }
    return true;
}

bool yy_map_foreach(yy_map_t *map, yy_map_foreach_func func, void *context) {
    long i;
    yy_map_node_t **bucket, *node;
    
    if (func == NULL) return false;
    for (i = 0; i < map->bucket_count; i++) {
        bucket = &map->buckets[i];
        node = *bucket;
        while (node) {
            func(node->key, node->value, context);
            node = node->next;
        }
    }
    return true;
}

yy_array_t *yy_map_create_key_array(yy_map_t *map) {
    yy_array_callback_t callback;
    yy_array_t *array;
    long i;
    yy_map_node_t **bucket, *node;
    
    callback.retain = map->key_callback.retain;
    callback.release = map->key_callback.release;
    callback.equal = map->key_callback.equal;
    array = yy_array_create_with_options(0, &callback);
    if (array == NULL) return NULL;
    for (i = 0; i < map->bucket_count; i++) {
        bucket = &map->buckets[i];
        node = *bucket;
        while (node) {
            yy_array_append(array, node->key);
            node = node->next;
        }
    }
    return array;
}
