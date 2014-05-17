//
//  main.c
//
//  Copyright (c) 2014 ibireme. All rights reserved.
//

#include <CoreFoundation/CoreFoundation.h>

#include "yy_array.h"
#include "yy_map.h"


static inline void profile_time(void (^block)(void), void (^complete)(double ms)) {
	struct timeval t0, t1;
	gettimeofday(&t0, NULL);
	block();
	gettimeofday(&t1, NULL);
    
	double ms = (double)(t1.tv_sec - t0.tv_sec) * 1e3 + (double)(t1.tv_usec - t0.tv_usec) * 1e-3;
	complete(ms);
}


////////////////////////////////////////////////////////////////////////////////
///                               Test Array                                 ///
////////////////////////////////////////////////////////////////////////////////

void test_array() {
    
    /// create yy array
	yy_array_callback_t yy_callback;
	yy_callback.retain = (yy_array_retain_callback)CFRetain;
	yy_callback.release = (yy_array_release_callback)CFRelease;
	yy_callback.equal = (yy_array_equal_callback)CFEqual;
    yy_array_t *yy_array = yy_array_create_with_options(0, &yy_callback);
    
    /// create CFArray
    CFArrayCallBacks cf_callback;
    cf_callback = kCFTypeArrayCallBacks;
    CFMutableArrayRef cf_array = CFArrayCreateMutable(CFAllocatorGetDefault(), 0, &cf_callback);
    
    /// create object (used for insert)
    CFDataRef obj = CFDataCreate(CFAllocatorGetDefault(), NULL, 0);
    
    
    printf("--------------------------------\n");
    printf("        prepend object\n");
    printf("--------------------------------\n");
    printf("count\t|yy_array\t|CFArray\n");
    
	for (int r = 1; r <= 3; r++) {
		int count = r * 100000;
		printf("%d\t|", count);
		profile_time(^ {
            for (int i = 1; i <= count; i++) {
                yy_array_prepend(yy_array, obj);
            }
        }, ^ (double ms) {
            printf("%.2fms\t\t|", ms);
        });
        
		profile_time(^ {
            for (int i = 1; i <= count; i++) {
                CFArrayInsertValueAtIndex(cf_array, 0, obj);
            }
        }, ^ (double ms) {
            printf("%.2fms\n", ms);
        });
        
        
		yy_array_clear(yy_array);
		CFArrayRemoveAllValues(cf_array);
	}
    
    printf("\n\n");
    printf("--------------------------------\n");
    printf("      random insert object\n");
    printf("--------------------------------\n");
    printf("count\t|yy_array\t|CFArray\n");
    
	for (int r = 1; r <= 3; r++) {
		int count = r * 100000;
		printf("%d\t|", count);
		profile_time(^ {
            int i, index;
            for (i = 1; i <= count; i++) {
                index = arc4random() % i;
                yy_array_insert(yy_array, index, obj);
            }
        }, ^ (double ms) {
            printf("%.2fms\t|", ms);
        });
        
		profile_time(^ {
            int i, index;
            for (i = 1; i <= count; i++) {
                index = arc4random() % i;
                CFArrayInsertValueAtIndex(cf_array, index, obj);
            }
        }, ^ (double ms) {
            printf("%.2fms\n", ms);
        });
        
		yy_array_clear(yy_array);
		CFArrayRemoveAllValues(cf_array);
	}
    
    
    yy_release(yy_array);
    CFRelease(cf_array);
    CFRelease(obj);
    
    printf("\n");
}


int main(int argc, const char * argv[]) {
    test_array();
    CFShow(CFSTR("Done!\n"));
    return 0;
}

