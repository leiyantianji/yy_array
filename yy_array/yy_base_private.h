//
//  yy_base_private.h
//  YYMidiBase
//
//  Created by ibireme on 14-2-25.
//  Copyright (c) 2014 ibireme. All rights reserved.
//

#ifndef YYMidiBase_yy_base_private_h
#define YYMidiBase_yy_base_private_h

#undef	YY_MAX
#define YY_MAX(a, b)  (((a) > (b)) ? (a) : (b))

#undef	YY_MIN
#define YY_MIN(a, b)  (((a) < (b)) ? (a) : (b))

#undef	YY_ABS
#define YY_ABS(a)	   (((a) < 0) ? -(a) : (a))

#undef	YY_CLAMP
#define YY_CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))


typedef struct _yy_object yy_object;

struct _yy_object {
    long ref_count;
    void *(*dealloc)(void *);
    /* object struct */
};

void *_yy_alloc(size_t size, void *(*dealloc)(void *));
void _yy_dealloc(void *object);

#define yy_alloc(type, dealloc) _yy_alloc(sizeof(type),(void *(*)(void *))(dealloc))
#define yy_dealloc(object) _yy_dealloc(object);


#endif
