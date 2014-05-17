//
//  yy_array.c
//  YYMidiBase
//
//  Created by ibireme on 14-2-14.
//  Copyright (c) 2014 ibireme. All rights reserved.
//

#if 0 /// no longer use deque, see `yy_array.c`

#include "yy_array.h"
#include "yy_base_private.h"
#include "yy_log.h"
#include "yy_sort.h"

#include <string.h>
#include <limits.h>

struct _yy_array {
	long count;
	long capacity;
	long index;
	const void **store;
	yy_array_callback_t callback;
};


static inline bool _yy_array_validate_range(yy_array_t *array, yy_range range, const char *func) {
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

static inline bool _yy_array_validate_index(yy_array_t *array, long index, bool edge, const char *func) {
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

static inline long _yy_array_capacity_expand(long capacity) {
	//    int i = 1;
	//    if (capacity < 16) return 16;
	//    else if (capacity >= (LONG_MAX >> 1)) return LONG_MAX;
	//    while ((capacity >>= 1)) i++;
	//    return 1L << i;
    
	if (capacity < 4) return 4;
	return YY_MIN((1L << flsl(capacity)), LONG_MAX);
}

static void _yy_array_reposition_deque_regions(yy_array_t *array,
                                               yy_range    range,
                                               long        new_length) {
	long old_count = array->count;
	long new_count = old_count - range.length + new_length;
    
	long l_free = array->index;
	long l_used = range.location;
	long old_length = range.length;
	long r_used = old_count - l_used - old_length;
	long r_free = array->capacity - l_used - old_count;
    
	long new_expand = new_length - old_length;
	long new_index;
	const void **deque = array->store;
	const void **new_deque;
	long new_capacity;
    
    
	long min_capacity = new_count * 1.2;
	if (new_count > array->capacity ||
	    (old_count < new_count && array->capacity < min_capacity)) {
		new_capacity = array->capacity << 1;
		if (new_capacity <= 0) {
			new_capacity = LONG_MAX;
		} else {
			while (new_capacity < min_capacity) new_capacity <<= 1;
		}
        
		new_deque = malloc(new_capacity * sizeof(void *));
		new_index = (new_capacity - new_count) / 2;
		if (l_used > 0)
			memmove(new_deque + new_index,
			        deque + l_free,
			        l_used * sizeof(void *));
        
		if (r_used > 0)
			memmove(new_deque + new_index + l_used + new_length,
			        deque + l_free + l_used + old_length,
			        r_used * sizeof(void *));
        
		free(array->store);
		array->store = new_deque;
		array->index = new_index;
		array->capacity = new_capacity;
		return;
	}
    
	if ((new_expand < 0 || new_expand <= l_free) && l_used < r_used) {
		if (l_used > 0)
			memmove(deque + l_free - new_expand,
			        deque + l_free,
			        l_used * sizeof(void *));
		array->index = l_free - new_expand;
	} else if ((new_expand < 0 || new_expand <= r_free) && r_used < l_used)   {
		if (r_used > 0)
			memmove(deque + l_free + l_used + new_length,
			        deque + l_free + l_used + old_length,
			        r_used * sizeof(void *));
	} else {
		// now, must be inserting, and either:
		//    A<=C, but L doesn't have room (R might have, but don't care)
		//    C<A, but R doesn't have room (L might have, but don't care)
		// re-center everything
        
		long newL = array->capacity - l_used - r_used - new_expand;
		newL = newL / 2;
		array->index = newL;
        
		if (newL < l_free) {
			if (l_used > 0)
				memmove(deque + newL,
				        deque + l_free,
				        l_used * sizeof(void *));
			if (r_used > 0)
				memmove(deque + newL + l_used + new_length,
				        deque + l_free + l_used + old_length,
				        r_used * sizeof(void *));
		} else {
			if (r_used > 0)
				memmove(deque + newL + l_used + new_length,
				        deque + l_free + l_used + old_length,
				        r_used * sizeof(void *));
			if (l_used > 0)
				memmove(deque + newL,
				        deque + l_free,
				        l_used * sizeof(void *));
		}
	}
}

static bool _yy_array_replace_values(yy_array_t *array, yy_range range, const void **new_values, long new_length) {
	int i;
	const void *buffer[64];
	const void **new_values_retained;
	bool retained_need_free;
    
	long old_count = array->count;
	long new_count = old_count - range.length + new_length;
	long old_capacity = array->capacity;
	long new_capacity;
    
	/**************************** alloc memory ********************************/
	if (array->store == NULL && new_count > 0) {
		new_capacity = _yy_array_capacity_expand(new_count);
		array->store = malloc(new_capacity * sizeof(void *));
		if (array->store == NULL) {
			yy_log_error("yy_array_t(%p):%s() attempt to allocate %ld bytes failed",
			             array, __func__, new_capacity * sizeof(void *));
			return false;
		}
		array->index = (new_capacity - new_length) / 2;
		array->capacity = new_capacity;
	}
    
    
	/**************************** retain new value ****************************/
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
    
	/**************************** release old value ***************************/
	if (range.length > 0 && array->callback.release) {
		for (i = 0; i < range.length; i++) {
			array->callback.release(array->store[array->index + range.location + i]);
		}
	}
    
    
	/**************************** move rest value *****************************/
	if (old_capacity > 0 && range.length != new_length) {
		_yy_array_reposition_deque_regions(array, range, new_length);
	}
    
    
	/**************************** move new value ******************************/
	if (new_length > 0) {
		memmove(array->store + array->index + range.location,
		        new_values_retained,
		        new_length * sizeof(void *));
	}
    
    
	/**************************** finish **************************************/
	if (new_count == 0 && array->store) {
		free(array->store);
		array->index = 0;
		array->store = NULL;
		array->capacity = 0;
	}
	if (retained_need_free)
		free(new_values_retained);
    
	array->count = new_count;
	return true;
}

yy_array_t *yy_array_create() {
	yy_array_t *array;
    
	array = calloc(1, sizeof(yy_array_t));
	if (array == NULL) {
		yy_log_error("yy_array_t(%p):%s() attempt to allocate %ld bytes failed",
		             array, __func__, sizeof(yy_array_t));
		return NULL;
	}
	return array;
}

yy_array_t *yy_array_create_with_options(long capacity, const yy_array_callback_t *callback) {
	yy_array_t *array;
    
	if (capacity < 0) {
		yy_log_error("%s() capacity(%ld) cannot be less than zero",
		             __func__, capacity);
		return NULL;
	}
	array = calloc(1, sizeof(yy_array_t));
	if (array == NULL) {
		yy_log_error("yy_array_t(%p):%s() attempt to allocate %ld bytes failed",
		             array, __func__, sizeof(yy_array_t));
		return NULL;
	}
    
	if (callback) {
		array->callback = *callback;
	}
	if (capacity > 0) {
		capacity = _yy_array_capacity_expand(capacity);
		array->store = malloc(capacity * sizeof(void *));
		if (array->store == NULL) {
			free(array);
			yy_log_error("yy_array_t(%p):%s() attempt to allocate %ld bytes failed",
			             array, __func__, capacity * sizeof(void *));
			return NULL;
		}
		array->capacity = capacity;
		array->index = capacity / 2;
	}
	return array;
}

void yy_array_release(yy_array_t *array) {
	int i;
	const void *item;
    
	if (array == NULL) return;
	if (array->count > 0 && array->callback.release) {
		item = array->store[array->index];
		for (i = 0; i < array->count; i++) {
			array->callback.release(item);
			item++;
		}
	}
	if (array->store) free(array->store);
	free(array);
}

const void *yy_array_get(yy_array_t *array, long index) {
	if (!_yy_array_validate_index(array, index, false, __func__)) {
		return false;
	}
	return array->store[array->index + index];
}

long yy_array_count(yy_array_t *array) {
	return array->count;
}

bool yy_array_get_range(yy_array_t *array, yy_range range, const void **values) {
	const void *src;
    
	if (!_yy_array_validate_range(array, range, __func__)) {
		return false;
	}
	if (range.length == 0) {
		return true;
	}
	src = array->store + array->index + range.location;
	memmove(values, &src, range.length);
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
	if (array->callback.retain) {
		value = array->callback.retain(value);
	}
	if (array->callback.release) {
		array->callback.release(array->store[array->index + index]);
	}
	array->store[array->index + index] = value;
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
	tmp = array->store[array->index + index1];
	array->store[array->index + index1] = array->store[array->index + index2];
	array->store[array->index + index2] = tmp;
	return true;
}

bool yy_array_clear(yy_array_t *array) {
	long i;
	const void *item;
    
	if (array->store == NULL) {
		return true;
	}
	if (array->callback.release) {
		item = array->store[array->index];
		for (i = 0; i < array->count; i++) {
			array->callback.release(item);
			item++;
		}
	}
	free(array->store);
	array->capacity = 0;
	array->count = 0;
	array->store = NULL;
	return true;
}

bool yy_array_contains(yy_array_t *array, const void *value) {
	int i;
	const void **item;
    
	i = 0;
	item = array->store + array->index;
	while (i < array->count) {
		if (*item == value
		    || (array->callback.equal && array->callback.equal(*item, value))) {
			return true;
		}
		item++;
		i++;
	}
	return false;
}

long yy_array_get_first_index(yy_array_t *array, yy_range range, const void *value) {
	long index, end;
	const void *item;
    
	if (!_yy_array_validate_range(array, range, __func__) || range.length == 0) {
		return YY_NOT_FOUND;
	}
	end = array->index + range.location + range.length - 1;
	index = array->index + range.location;
	for (; index <= end; index++) {
		item = array->store[index];
		if (item == value
		    || (array->callback.equal && array->callback.equal(item, value))) {
			return index - array->index;
		}
	}
	return YY_NOT_FOUND;
}

long yy_array_get_last_index(yy_array_t *array, yy_range range, const void *value) {
	long index, head;
	const void *item;
    
	if (!_yy_array_validate_range(array, range, __func__) || range.length == 0) {
		return YY_NOT_FOUND;
	}
	head = array->index + range.location;
	index = head + range.length - 1;
	for (; index >= head; index--) {
		item = array->store[index];
		if (item == value
		    || (array->callback.equal && array->callback.equal(item, value))) {
			return index - array->index;
		}
	}
	return YY_NOT_FOUND;
}

bool yy_array_sort(yy_array_t *array, yy_range range, yy_comparator_func comparator, void *context) {
	if (!_yy_array_validate_range(array, range, __func__)) {
		return false;
	}
	yy_quick_sort(array->store + array->index + range.location, range.length, comparator, context);
	return true;
}

bool yy_array_foreach(yy_array_t *array, yy_array_foreach_func func, void *context) {
	long i;
	const void **item;
    
	if (!func) return false;
	i = 0;
	item = array->store + array->index;
	while (i < array->count) {
		func(i, *item, context);
		item++;
		i++;
	}
	return true;
}

bool yy_array_foreach_range(yy_array_t *array, yy_range range, yy_array_foreach_func func, void *context) {
	long i;
	const void **item;
    
	if (!func) return false;
	if (!_yy_array_validate_range(array, range, __func__)) return false;
	i = range.location;
	item = array->store + array->index + range.location;
	while (i < range.location + range.length) {
		func(i, *item, context);
		item++;
		i++;
	}
	return true;
}


#endif