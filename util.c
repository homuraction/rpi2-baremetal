#include "util.h"
#include "sysinfo.h"
#include "cpu.h"

/*
volatile int busy_wait(int32_t count) {
        volatile int32_t cnt = count;
        while(cnt > 0) {
                cnt--;
        }
        return 0;
}
*/

/* heap includes sential */

void heap_init (HEADER *base, uint32_t heap_size)
{
        uint32_t num_heap_units = heap_size / HEAP_UNIT_SIZE;
        base = memset (base, 0, sizeof(HEADER) * num_heap_units);
}

void *memset (void *b, int c, uint32_t len)
{
        char *p = (char *)b;

        while (len--)
                *p++ = c;
        return b;
}

void *memcpy (void *dst, const void *src, uint32_t n)
{
        char *p1 = (char *)dst;
        const char *p2 = (const char *)src;
        while (n--)
                *p1++ = *p2++;
        return dst;
}

void *
kmalloc(const size_t nbytes)
{
        const uint32_t pm        = get_pm();
        const uint32_t cpuid     = _get_prid();
        const uint32_t HEAP_SIZE = get_kernel_heap_limit(cpuid, pm) -
                get_kernel_heap_bottom(cpuid, pm);

        /* base of free list */
        HEADER *base  = (HEADER *)get_kernel_heap_bottom(cpuid, pm);
        /* start of free list */
        HEADER *freep = (HEADER *)get_kernel_heap_free_head(cpuid, pm);

        return _malloc(base, freep, nbytes);

}

//void *
//malloc(const size_t nbytes)
//{
//        const uint32_t pm        = get_pm();
//        const uint32_t cpuid     = _get_prid();
//        const uint32_t HEAP_SIZE = get_user_heap_limit(cpuid, pm) -
//                get_user_heap_bottom(cpuid, pm);
//
//        /* base of free list */
//        HEADER *base  = (HEADER *)get_user_heap_bottom(cpuid, pm);
//        /* start of free list */
//        HEADER *freep = (HEADER *)get_user_heap_free_head(cpuid, pm);
//
//        return _malloc(base, freep, nbytes);
//}

void *
_malloc(HEADER *base, HEADER *freep, uint32_t nbytes)
{
        HEADER *prevp, *p;

        // size is aligned by (HEAP_UNIT_SIZE - 1)
        const uint32_t nunits =
                (nbytes + HEAP_UNIT_SIZE - 1) / HEAP_UNIT_SIZE + 1;

        if ((prevp = freep) == NULL) { /* no free list */
                base->s.ptr = freep = prevp = base;
                base->s.size = 0;
        }

        for (p = prevp->s.ptr;; prevp = p, p = p->s.ptr) {
                if (p->s.size >= nunits) {       /* big enough */
                        if (p->s.size == nunits) /* exactly */
                                prevp->s.ptr = p->s.ptr;
                        else {                   /* allocate tail end */
                                p->s.size -= nunits;
                                p         += p->s.size;
                                p->s.size  = nunits;
                        }

                        freep = prevp;
                        return ((void *)(p + 1));
                }

                if (p == freep)
                        return NULL;             /* none left */
        }

        return NULL;
}

void
kfree (void *ap)
{
        const uint32_t pm        = get_pm();
        const uint32_t cpuid     = _get_prid();

        /* start of free list */
        HEADER *freep = (HEADER *)get_kernel_heap_free_head(cpuid, pm);

        _free(freep, ap);
}

//void
//free (void *ap)
//{
//        const uint32_t pm        = get_pm();
//        const uint32_t cpuid     = _get_prid();
//
//        /* start of free list */
//        HEADER *freep = (HEADER *)get_user_heap_free_head(cpuid, pm);
//
//        _free(freep, ap);
//}

void
_free (HEADER *freep, void *ap)
{
        HEADER *prevp, *p;

        p = (HEADER *)ap - 1;

        for (prevp = freep                   ;
             !(p > prevp && p < prevp->s.ptr);
             prevp = prevp->s.ptr             ) {
                if (prevp >= prevp->s.ptr && (p > prevp || p < prevp->s.ptr))
                        break;
        }

        if (p + p->s.size == prevp->s.ptr) {
                p->s.size += prevp->s.ptr->s.size;
                p->s.ptr   = prevp->s.ptr->s.ptr;
        } else {
                p->s.ptr   = prevp->s.ptr;
        }

        if (prevp + prevp->s.size == p) {
                prevp->s.size += p->s.size;
                prevp->s.ptr   = p->s.ptr;
        } else
                prevp->s.ptr = p;

        freep = prevp;
}
