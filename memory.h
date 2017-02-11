#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "stdint.h"

/* Memory type attributes */
#define MEMTYPE_STRORD // Shareable; Strongly ordered memory
#define MEMTYPE_DEVICE // Peripheral
#define MEMTYPE_NORMAL // Normal memory; Has cache attribute

/* Shareability */
#define SHAREABLE
#define UNSHAREABLE
#define OUTSHAREABLE
#define INSHAREABLE

/* Cache attributes */
#define UNCACHEABLE
#define WRITETHROUGH
#define WRITEBACK_ALLOCATE
#define WRITEBACK_NOALLOCATE

#endif /* __MEMORY_H__ */
