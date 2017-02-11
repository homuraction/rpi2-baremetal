#ifndef __INIT_H__
#define __INIT_H__

#include "stdint.h"

void sys_init_primary(void);
void sys_init_secondary(void);
void init_ttbs(void);
void init_smp_kernel(void);
void init_amp_kernel(void);

#endif /* __INIT_H__ */
