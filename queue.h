#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "stdint.h"

typedef struct wque {
        struct wque *next;
        struct wque *prev;
} wait_que_t;

#endif /* __QUEUE_H__ */
