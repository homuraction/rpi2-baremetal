#ifndef __MULTICORE_H__
#define __MULTICORE_H__

#include "stdint.h"

#define CORES 4

uint32_t start_secondary_core (void);
uint32_t start_core (void);
void ipi_handler (uint32_t n_core, uint32_t n_ipi);
void send_ipi (uint32_t n_core, uint32_t n_ipi);
uint32_t local_int_handler (void);

#endif /* __MULTICORE_H__ */
