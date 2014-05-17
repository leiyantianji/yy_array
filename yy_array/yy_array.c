//
//  yy_array.c
//  YYMidiBase
//
//  Created by ibireme on 14-2-14.
//  Copyright (c) 2014 ibireme. All rights reserved.
//

#include "yy_array.h"
#include "yy_base_private.h"
#include "yy_log.h"
#include "yy_sort.h"

#include <string.h>
#include <limits.h>


static bool _yy_array_string_equal_callback(const void *key1, const void *key2) {
    return strcmp(key1, key2) == 0;
}

yy_array_callback_t yy_array_string_callback = {
    (yy_array_retain_callback)strdup,
    (yy_array_release_callback)free,
    _yy_array_string_equal_callback,
};

yy_array_callback_t yy_array_object_callback = {
    (yy_array_retain_callback)yy_retain,
    (yy_array_release_callback)yy_release,
    NULL,
};

struct _yy_array {
    long count;
    long capacity;
    long index;
    const void **ring;
    yy_array_callback_t callback;
};

/**
 * Validate range and log error.
 *
 * @param range  the range of array
 */
yy_inline bool _yy_array_validate_range(yy_array_t *array, yy_range range, const char *func) {
    if (range.location < 0 || range.location > array->count) {
        yy_log_error("yy_array_t(%p):%s() range.location(%ld) out of bounds(0,%ld)",
                     array, func, range.location, array->count);
        return false;
    }
    if (range.length < 0) {
        yy_log_error("yy_array_t(%p):%s() range.length(%ld) cannot be less than zero",
                     array, func, range.length);
        return false;
    }
    if (range.location + range.length > array->count) {
        yy_log_error("yy_array_t(%p):%s() range(%ld,%ld) ending index out of bounds(0,%ld)",
                     array, func, range.location, range.length, array->count);
        return false;
    }
    return true;
}

/**
 * Validate index and log error.
 *
 * @param index the index of array
 * @param edge  the index can be edge
 */
yy_inline bool _yy_array_validate_index(yy_array_t *array, long index, bool edge, const char *func) {
    if (edge) {
        if (index < 0 || index > array->count) {
            yy_log_error("yy_array_t(%p):%s() index(%ld) out of bounds(0,%ld)",
                         array, func, index, array->count);
            return false;
        }
    } else {
        if (array->count == 0) {
            yy_log_error("yy_array_t(%p):%s() is empty",
                         array, func, index, array->count - 1);
            return false;
        }
        if (index < 0 || index >= array->count) {
            yy_log_error("yy_array_t(%p):%s() index(%ld) out of bounds(0,%ld)",
                         array, func, index, array->count - 1);
            return false;
        }
    }
    return true;
}

/**
 * Expand capacity to fit 2^x.
 */
yy_inline long _yy_array_capacity_expand(long capacity) {
    int i = 1;
    if (capacity < 16) return 16;
    else if (capacity >= (LONG_MAX >> 1)) return LONG_MAX;
    while ((capacity >>= 1)) i++;
    return 1L << i;
}

/**
 * Split ring's range to absolute offset.
 *
 * @param range the range of array
 * @param abs1  absolute offset 1
 * @param abs2  absolute offset 2
 */
yy_inline void _yy_array_split(yy_array_t *array, yy_range range, yy_range *abs1, yy_range *abs2) {
    abs1->location = array->index + range.location;
    abs1->length = range.length;
    while (abs1->location >= array->capacity) abs1->location -= array->capacity;
    while (abs1->location < 0) abs1->location += array->capacity;
    
    abs2->location = abs1->location + range.length;
    if (abs2->location >= array->capacity) {
        abs2->location = 0;
        abs2->length = abs1->location + abs1->length - array->capacity;
        abs1->length -= abs2->length;
    } else {
        abs2->length = 0;
    }
}

/**
 * Move the specified range of memory to left or right.
 *
 * @param range absolute offset range
 * @param move  negative value means move left, 
 *              positive value means move right.
 */
yy_inline void _yy_array_move_range(yy_array_t *array, yy_range range, long move) {
    /*
        move to the left    |    move to the right
     -----------------------+-----------------------------------
     1. [       ABC    ]    |    [    ABC       ]        before
        [    ABC       ]    |    [       ABC    ]        after
     -----------------------+-----------------------------------
     2. [ ABC          ]    |    [          ABC ]        before
        [BC           A]    |    [C           AB]        after
     -----------------------+-----------------------------------
     3. [BC           A]    |    [C           AB]        before
        [          ABC ]    |    [ ABC          ]        after
     -----------------------+-----------------------------------
     4. [BC           A]    |    [C           AB]        before
        [C           AB]    |    [BC           A]        after
     -----------------------------------------------------------
     */
    yy_range src1, src2, dest1, dest2;
    
    _yy_array_split(array, range, &src1, &src2);
    range.location += move;
    _yy_array_split(array, range, &dest1, &dest2);
    
    if (src2.length == 0 && dest2.length == 0) {            /** 1. */
        memmove(array->ring + dest1.location,
                array->ring + src1.location,
                src1.length * sizeof(void *));
    } else if (src2.length == 0 && dest2.length > 0) {      /** 2. */
        if (move < 0) {                                     /** 2.to left */
            memmove(array->ring + dest1.location,
                    array->ring + src1.location,
                    dest1.length * sizeof(void *));
            memmove(array->ring + dest2.location,
                    array->ring + src1.location + dest1.length,
                    dest2.length * sizeof(void *));
        } else {                                            /** 2.to right */
            memmove(array->ring + dest2.location,
                    array->ring + src1.location + dest1.length,
                    dest2.length * sizeof(void *));
            memmove(array->ring + dest1.location,
                    array->ring + src1.location,
                    dest1.length * sizeof(void *));
        }
    } else if (dest2.length == 0 && src2.length > 0) {      /** 3. */
        if (move < 0) {                                     /** 3.to left */
            memmove(array->ring + dest1.location,
                    array->ring + src1.location,
                    src1.length * sizeof(void *));
            memmove(array->ring + dest1.location + src1.length,
                    array->ring + src2.location,
                    src2.length * sizeof(void *));
        } else {                                            /** 3.to right */
            memmove(array->ring + dest1.location + src1.length,
                    array->ring + src2.location,
                    src2.length * sizeof(void *));
            memmove(array->ring + dest1.location,
                    array->ring + src1.location,
                    src1.length * sizeof(void *));
        }
    } else if (dest2.length > 0 && src2.length > 0) {       /** 4. */
        if (move < 0) {                                     /** 4.to left */
            memmove(array->ring + dest1.location,
                    array->ring + src1.location,
                    src1.length * sizeof(void *));
            memmove(array->ring + dest1.location + src1.length,
                    array->ring + src2.location,
                    (-move) * sizeof(void *));
            memmove(array->ring + dest2.location,
                    array->ring + src2.location - move,
                    dest2.length * sizeof(void *));
        } else {                                            /** 4.to right */
            memmove(array->ring + dest2.location + move,
                    array->ring + src2.location,
                    src2.length * sizeof(void *));
            memmove(array->ring + dest2.location,
                    array->ring + src1.location + dest1.length,
                    move * sizeof(void *));
            memmove(array->ring + dest1.location,
                    array->ring + src1.location,
                    (src1.length - move) * sizeof(void *));
        }
    }
}

yy_inline void _yy_array_release_range(yy_array_t *array, yy_range range) {
    const void **item;
    int i;
    yy_range src1, src2;
    
    _yy_array_split(array, range, &src1, &src2);
    if (src1.length > 0) {
        item = array->ring + src1.location;
        for (i = 0; i < src1.length; i++, item++) {
            array->callback.release(*item);
        }
    }
    if (src2.length > 0) {
        item = array->ring + src2.location;
        for (i = 0; i < src2.length; i++, item++) {
            array->callback.release(*item);
        }
    }
}

static bool _yy_array_reposition_ring_regions(yy_array_t *array, yy_range range,long new_length) {
    const void **new_ring;
    long old_count, old_capacity, new_capacity, new_index, move;
    long l_used, r_used, old_length;
    yy_range src1, src2;
    
    old_count = array->count;
    l_used = range.location;
    old_length = range.length;
    r_used = old_count - l_used - old_length;
    
    old_capacity = array->capacity;
    new_capacity = array->count - range.length + new_length;
    
    if (new_capacity >= old_capacity) {
        new_capacity = _yy_array_capacity_expand(new_capacity);
        new_index = 0;
        new_ring = malloc(new_capacity * sizeof(void *));
        if (new_ring == NULL) {
            yy_log_error("yy_array_t(%p):%s() attempt to allocate %ld bytes failed",
                         array, __func__, new_capacity * sizeof(void *));
            return false;
        }

        if (l_used > 0) {
            _yy_array_split(array, yy_range_make(0, l_used), &src1, &src2);
            if (src1.length > 0) {
                memcpy(new_ring + new_index,
                       array->ring + src1.location,
                       src1.length * sizeof(void *));
            }
            if (src2.length > 0) {
                memcpy(new_ring + new_index + src1.length,
                       array->ring + src2.location,
                       src2.length * sizeof(void *));
            }
        }
        if (r_used > 0) {
            _yy_array_split(array, yy_range_make(l_used + old_length, r_used), &src1, &src2);
            if (src1.length > 0) {
                memcpy(new_ring + new_index + l_used + new_length,
                       array->ring + src1.location,
                       src1.length * sizeof(void *));
            }
            if (src2.length > 0) {
                memcpy(new_ring + new_index + l_used + new_length + src1.length,
                       array->ring + src2.location,
                       src2.length * sizeof(void *));
            }
        }
        
        free(array->ring);
        array->ring = new_ring;
        array->index = new_index;
        array->capacity = new_capacity;
        return true;
    }
    
    if (l_used < r_used) {
        move = old_length - new_length;
        _yy_array_move_range(array, yy_range_make(0, l_used), move);
        new_index = array->index + move;
        while (new_index < 0) new_index += array->capacity;
        while (new_index >= array->capacity) new_index -= array->capacity;
    } else {
        move = new_length - old_length;
        _yy_array_move_range(array, yy_range_make(l_used + old_length, r_used), move);
        new_index = array->index;
    }
    array->index = new_index;
    return true;
}

static bool _yy_array_replace_values(yy_array_t *array, yy_range range, const void **new_values, long new_length) {
    const void *buffer[64];
    const void **new_values_retained;
    bool retained_need_free;
    bool result;
    long i, old_count, new_count, old_capacity, new_capacity;
    yy_range dest1, dest2;
    
    old_count = array->count;
    new_count = old_count - range.length + new_length;
    old_capacity = array->capacity;
    
    /**************************** alloc memory ********************************/
    if (array->ring == NULL && new_count > 0) {
        new_capacity = _yy_array_capacity_expand(new_count);
        array->ring = malloc(new_capacity * sizeof(void *));
        if (array->ring == NULL) {
            yy_log_error("yy_array_t(%p):%s() attempt to allocate %ld bytes failed",
                         array, __func__, new_capacity * sizeof(void *));
            return false;
        }
        array->index = 0;
        array->capacity = new_capacity;
    }
    
    /**************************** retain and release **************************/
    retained_need_free = false;
    if (new_length > 0 && array->callback.retain) {
        if (new_length <= 64) {
            new_values_retained = buffer;
        } else {
            new_values_retained = malloc(new_length * sizeof(void *));
            if (new_values_retained == NULL) {
                yy_log_error("yy_array_t(%p):%s() attempt to allocate %ld bytes failed",
                             array, __func__, new_length * sizeof(void *));
                return false;
            }
            retained_need_free = true;
        }
        for (i = 0; i < new_length; i++) {
            new_values_retained[i] = array->callback.retain(new_values[i]);
        }
    } else {
        new_values_retained = new_values;
    }
    
    if (range.length > 0 && array->callback.release) {
        _yy_array_release_range(array, range);
    }
    
    /**************************** resposition regions *************************/
    if (old_capacity > 0 && range.length != new_length) {
        result = _yy_array_reposition_ring_regions(array, range, new_length);
        if (!result && retained_need_free) {
            free(new_values_retained);
            return false;
        }
    }
    
    /**************************** copy new value ******************************/
    if (new_length > 0) {
        _yy_array_split(array, yy_range_make(range.location, new_length), &dest1, &dest2);
        if (dest1.length > 0) {
            memmove(array->ring + dest1.location,
                    new_values_retained,
                    dest1.length * sizeof(void *));
        }
        if (dest2.length > 0) {
            memmove(array->ring + dest2.location,
                    new_values_retained + dest1.length,
                    dest2.length * sizeof(void *));
        }
    }
    
    /**************************** finish **************************************/
    if (new_count == 0 && array->ring) {
        free(array->ring);
        array->index = 0;
        array->ring = NULL;
        array->capacity = 0;
    }
    if (retained_need_free) free(new_values_retained);
    
    array->count = new_count;
    return true;
}

static void _yy_array_dealloc(yy_array_t *array) {
    if (array->callback.release && array->count > 0) {
        _yy_array_release_range(array, yy_range_make(0, array->count));
    }
    if (array->ring) {
        free(array->ring);
    }
    yy_dealloc(array);
}

yy_array_t * yy_array_create() {
    return yy_array_create_with_options(0, NULL);
}

yy_array_t * yy_array_create_for_string() {
    return yy_array_create_with_options(0, &yy_array_string_callback);
}

yy_array_t * yy_array_create_for_object() {
    return yy_array_create_with_options(0, &yy_array_object_callback);
}

yy_array_t * yy_array_create_with_options(long capacity, const yy_array_callback_t *callback) {
    yy_array_t *array;
    
    if (capacity < 0) {
        yy_log_error("%s() capacity(%ld) cannot be less than zero",
                     __func__, capacity);
        return NULL;
    }
    array = yy_alloc(yy_array_t, _yy_array_dealloc);
    
    if (array == NULL) {
        yy_log_error("yy_array_t:%s() attempt to allocate %ld bytes failed",
                     __func__, sizeof(yy_array_t));
        return NULL;
    }
    if (capacity > 0) {
        capacity = _yy_array_capacity_expand(capacity);
        array->ring = malloc(capacity * sizeof(void *));
        if (array->ring == NULL) {
            yy_dealloc(array);
            yy_log_error("yy_array_t:%s() attempt to allocate %ld bytes failed",
                         __func__, capacity * sizeof(void *));
            return NULL;
        }
        array->capacity = capacity;
    }
    
    if (callback) array->callback = *callback;
    return array;
}

yy_array_t * yy_array_create_copy(yy_array_t *array) {
    yy_array_t *new_array;
    yy_range src1, src2;
    long i;
    
    if (array == NULL) {
        yy_log_error("%s() input array cannot be null",
                     __func__);
        return NULL;
    }
    
    new_array = yy_alloc(yy_array_t, _yy_array_dealloc);
    if (new_array == NULL) {
        yy_log_error("yy_array_t:%s() attempt to allocate %ld bytes failed",
                     __func__, sizeof(yy_array_t));
        return NULL;
    }
    
    new_array->callback = array->callback;
    if (array->count > 0) {
        new_array->capacity = array->capacity;
        new_array->count = array->count;
        new_array->index = array->index;
        new_array->ring = malloc(array->capacity * sizeof(void *));
        if (new_array->ring == NULL) {
            yy_log_error("yy_array_t:%s() attempt to allocate %ld bytes failed",
                         __func__, array->capacity * sizeof(void *));
            yy_dealloc(array);
            return NULL;
        }
        
        _yy_array_split(array, yy_range_make(0, array->count), &src1, &src2);
        if (src1.length > 0) {
            memcpy(new_array->ring + src1.location,
                   array->ring + src1.location,
                   src1.length * sizeof(void *));
        }
        if (src2.length > 0) {
            memcpy(new_array->ring + src2.location,
                   array->ring + src2.location,
                   src2.length * sizeof(void *));
        }
        if (new_array->callback.retain != NULL) {
            for (i = src1.location; i < src1.location + src1.length; i++) {
                new_array->ring[i] = new_array->callback.retain(new_array->ring[i]);
            }
            for (i = src2.location; i < src2.location + src2.length; i++) {
                new_array->ring[i] = new_array->callback.retain(new_array->ring[i]);
            }
        }
    }
    
    return new_array;
}

const void * yy_array_get(yy_array_t *array, long index) {
    if (!_yy_array_validate_index(array, index, false, __func__)) {
        return NULL;
    }
    while (array->index  + index >= array->capacity) index -= array->capacity;
    return array->ring[array->index + index];
}

const void * yy_array_get_last(yy_array_t *array, long index) {
    index = array->count - index - 1;
    if (!_yy_array_validate_index(array, index, false, __func__)) {
        return NULL;
    }
    while (array->index  + index >= array->capacity) index -= array->capacity;
    return array->ring[array->index + index];
}

long yy_array_count(yy_array_t *array) {
    return array->count;
}

bool yy_array_get_range(yy_array_t *array, yy_range range, const void **values) {
    yy_range src1, src2;
    
    if (!_yy_array_validate_range(array, range, __func__)) {
        return false;
    }
    if (range.length == 0) {
        return true;
    }
    
    _yy_array_split(array, range, &src1, &src2);
    if (src1.length > 0) {
        memmove(values,
                array->ring + src1.location,
                src1.length * sizeof(void *));
    }
    if (src2.length > 0) {
        memmove(values + src1.length,
                array->ring + src2.location,
                src2.length * sizeof(void *));
    }
    return true;
}

bool yy_array_replace_range(yy_array_t *array, yy_range range, const void **new_values, long new_count) {
    if (!_yy_array_validate_range(array, range, __func__)) {
        return false;
    }
    if (new_count < 0) {
        yy_log_error("yy_array_t(%p):%s() new_count(%ld) cannot be less than zero",
                     array, __func__, new_count);
        return false;
    }
    return _yy_array_replace_values(array, range, new_values, new_count);
}

bool yy_array_append(yy_array_t *array, const void *value) {
    return _yy_array_replace_values(array, yy_range_make(array->count, 0), &value, 1);
}

bool yy_array_prepend(yy_array_t *array, const void *value) {
    return _yy_array_replace_values(array, yy_range_make(0, 0), &value, 1);
}

bool yy_array_set(yy_array_t *array, long index, const void *value) {
    if (!_yy_array_validate_index(array, index, false, __func__)) {
        return false;
    }
    while (array->index + index >= array->capacity) index -= array->capacity;
    if (array->callback.retain) {
        value = array->callback.retain(value);
    }
    if (array->callback.release) {
        array->callback.release(array->ring[array->index + index]);
    }
    array->ring[array->index + index] = value;
    return true;
}

bool yy_array_insert(yy_array_t *array, long index, const void *value) {
    if (!_yy_array_validate_index(array, index, true, __func__)) {
        return false;
    }
    return _yy_array_replace_values(array, yy_range_make(index, 0), &value, 1);
}

bool yy_array_remove(yy_array_t *array, long index) {
    if (!_yy_array_validate_index(array, index, false, __func__)) {
        return false;
    }
    return _yy_array_replace_values(array, yy_range_make(index, 1), NULL, 0);
}

bool yy_array_exchange(yy_array_t *array, long index1, long index2) {
    const void *tmp;
    
    if (!_yy_array_validate_index(array, index1, false, __func__)) {
        return false;
    }
    if (!_yy_array_validate_index(array, index2, false, __func__)) {
        return false;
    }
    
    while (array->index + index1 >= array->capacity) index1 -= array->capacity;
    while (array->index + index2 >= array->capacity) index2 -= array->capacity;
    
    tmp = array->ring[array->index + index1];
    array->ring[array->index + index1] = array->ring[array->index + index2];
    array->ring[array->index + index2] = tmp;
    return true;
}

bool yy_array_clear(yy_array_t *array) {
    if (array->ring == NULL) {
        return true;
    }
    if (array->callback.release && array->count > 0) {
        _yy_array_release_range(array, yy_range_make(0, array->count));
    }
    free(array->ring);
    array->capacity = 0;
    array->count = 0;
    array->ring = NULL;
    return true;
}

bool yy_array_contains(yy_array_t *array, const void *value) {
    return yy_array_get_first_index(array, yy_range_make(0, array->count), value) != YY_NOT_FOUND;
}

long yy_array_get_first_index(yy_array_t *array, yy_range range, const void *value) {
    long i, index;
    const void **item;
    yy_range src1, src2;
    
    if (!_yy_array_validate_range(array, range, __func__) || range.length == 0) {
        return YY_NOT_FOUND;
    }
    
    _yy_array_split(array, range, &src1, &src2);
    index = range.location;
    if (src1.length > 0) {
        item = array->ring + src1.location;
        for (i = 0; i < src1.length; i++, index++, item++) {
            if (*item == value
                || (array->callback.equal && array->callback.equal(*item, value))) {
                return index;
            }
        }
    }
    if (src2.length > 0) {
        item = array->ring + src2.location;
        for (i = 0; i < src2.length; i++, index++, item++) {
            if (*item == value
                || (array->callback.equal && array->callback.equal(*item, value))) {
                return index;
            }
        }
    }
    return YY_NOT_FOUND;
}

long yy_array_get_last_index(yy_array_t *array, yy_range range, const void *value) {
    long i, index;
    const void **item;
    yy_range src1, src2;
    
    if (!_yy_array_validate_range(array, range, __func__) || range.length == 0) {
        return YY_NOT_FOUND;
    }
    
    _yy_array_split(array, range, &src1, &src2);
    index = range.location + range.length - 1;
    if (src2.length > 0) {
        item = array->ring + src2.location;
        for (i = src2.length; i > 0; i--, index--, item--) {
            if (*item == value
                || (array->callback.equal && array->callback.equal(*item, value))) {
                return index;
            }
        }
    }
    if (src1.length > 0) {
        item = array->ring + src1.location + src1.length - 1;
        for (i = src1.length; i > 0; i--, index--, item--) {
            if (*item == value
                || (array->callback.equal && array->callback.equal(*item, value))) {
                return index;
            }
        }
    }
    return YY_NOT_FOUND;
}

bool yy_array_sort(yy_array_t *array, yy_comparator_func cmp, void *context) {
    return yy_array_sort_range(array, yy_range_make(0, array->count), cmp, context);
}

bool yy_array_sort_range(yy_array_t *array, yy_range range, yy_comparator_func cmp, void *context) {
    long move;
    
    if (!_yy_array_validate_range(array, range, __func__)) {
        return false;
    }
    if (range.length <= 1) return true;
    
    while (array->index + range.location >= array->capacity) range.location -= array->capacity;
    if (array->index + range.location + range.length > array->capacity) {
        move = -(array->index + range.location + range.length - array->capacity);
        _yy_array_move_range(array, range, move);
        array->index += move;
    }
    yy_quick_sort(array->ring + array->index + range.location, range.length, cmp, context);
    return true;
}

bool yy_array_foreach(yy_array_t *array, yy_array_foreach_func func, void *context) {
    return yy_array_foreach_range(array, yy_range_make(0, array->count), func, context);
}

bool yy_array_foreach_range(yy_array_t *array, yy_range range, yy_array_foreach_func func, void *context) {
    long i, index;
    const void **item;
    yy_range src1, src2;
    
    if (!func) return false;
    if (!_yy_array_validate_range(array, range, __func__)) return false;
    
    _yy_array_split(array, range, &src1, &src2);
    index = range.location;
    if (src1.length > 0) {
        item = array->ring + src1.location;
        for (i = 0; i < src1.length; i++, index++, item++) {
            func(index, *item, context);
        }
    }
    if (src2.length > 0) {
        item = array->ring + src2.location;
        for (i = 0; i < src2.length; i++, index++, item++) {
            func(index, *item, context);
        }
    }
    return true;
}
