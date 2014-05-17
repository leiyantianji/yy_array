//
//  yy_sort.c
//  YYMidiBase
//
//  Created by ibireme on 14-2-16.
//  Copyright (c) 2014 ibireme. All rights reserved.
//

#include "yy_sort.h"

#define _yy_sort_swap(x, y) { tmp = (x); (x) = (y); (y) = tmp; }

yy_inline long _yy_quick_sort_partition(const void         **dst,
                                        const long         left,
                                        const long         right,
                                        const long         pivot,
                                        yy_comparator_func cmpf,
                                        void               *context) {
    const void *value, *tmp;
    long index, i, all_same;
    
    value = dst[pivot];
    index = left;
    all_same = 1;
    
    /* move the pivot to the right */
    _yy_sort_swap(dst[pivot], dst[right]);
    
    for (i = left; i < right; i++) {
        yy_order cmp = cmpf(dst[i], value, context);
        /* check if everything is all the same */
        if (cmp != YY_ORDER_EQUAL) {
            all_same &= 0;
        }
        
        if (cmp == YY_ORDER_ASC) {
            _yy_sort_swap(dst[i], dst[index]);
            index++;
        }
    }
    _yy_sort_swap(dst[right], dst[index]);
    
    /* avoid degenerate case */
    if (all_same) {
        return -1;
    }
    
    return index;
}

static void _yy_quick_sort(const void         **dst,
                           const long         left,
                           const long         right,
                           yy_comparator_func cmp,
                           void               *context) {
    long pivot;
    long new_pivot;
    
    if (right <= left) {
        return;
    }
    pivot = left + ((right - left) >> 1);
    new_pivot = _yy_quick_sort_partition(dst, left, right, pivot, cmp, context);
    
    /* check for partition all equal */
    if (new_pivot < 0) {
        return;
    }
    _yy_quick_sort(dst, left, new_pivot - 1, cmp, context);
    _yy_quick_sort(dst, new_pivot + 1, right, cmp, context);
}

void yy_quick_sort(const void **values, const size_t size, yy_comparator_func cmp, void *context) {
    if (size <= 1 || values == NULL || cmp == NULL) return;
    _yy_quick_sort(values, 0, size - 1, cmp, context);
}
