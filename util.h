#ifndef __UTIL_H__
#define __UTIL_H__

#include "stdint.h"


typedef uint32_t size_t;
typedef long ALIGN;

typedef union header {
        struct {
                union header *ptr;
                unsigned      size;
        } s;
        ALIGN x;
} HEADER;

#define HEAP_UNIT_SIZE (sizeof(HEADER))

uint32_t allocsize(void *);

volatile int busy_wait(int32_t count);
void  heap_init (HEADER *base, uint32_t heap_size);
void *memset    (void *b, int c, uint32_t len);
void *memcpy    (void *dst, const void *src, uint32_t n);
void *kmalloc   (uint32_t nbytes);
void *malloc    (uint32_t nbytes);
void *_malloc   (HEADER *base, HEADER *freep, uint32_t nbytes);
void  kfree     (void *ap);
void  free      (void *ap);
void  _free     (HEADER *freep, void *ap);

#endif /* __UTIL_H__ */
