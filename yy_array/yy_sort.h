//
//  yy_sort.h
//  YYMidiBase
//
//  Created by ibireme on 14-2-16.
//  Copyright (c) 2014 ibireme. All rights reserved.
//

#ifndef YYMidiBase_yy_sort_h
#define YYMidiBase_yy_sort_h

#include <stdio.h>

#include "yy_base.h"

void yy_quick_sort(const void **values, const size_t size, yy_comparator_func cmp, void *context);

#endif
