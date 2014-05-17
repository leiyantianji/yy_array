//
//  yy_base.c
//  YYMidiBase
//
//  Created by ibireme on 14-2-20.
//  Copyright (c) 2014 ibireme. All rights reserved.
//

#include "yy_base.h"
#include "yy_base_private.h"

#include <stdio.h>

const void *yy_retain(void *object) {
    if (object) {
        yy_object *o = object;
        o--;
        o->ref_count++;
    }
    return object;
}

void yy_release(void *object) {
    if (object) {
        yy_object *o = object;
        o--;
        o->ref_count--;
        if (o->ref_count <= 0 && o->dealloc != NULL) {
            o->dealloc(object);
        }
    }
}

void *_yy_alloc(size_t size, void *(*dealloc)(void *)) {
    yy_object *o = calloc(1, sizeof(yy_object) + size);
    if (o) {
        o->ref_count = 1;
        o->dealloc = dealloc;
        return o + 1;
    }
    return NULL;
}

void _yy_dealloc(void *object) {
    yy_object *o = object;
    o--;
    free(o);
}

long yy_retain_count(void *object) {
    if (object) {
        yy_object *o = object;
        o--;
        return o->ref_count;
    }
    return 0;
}
