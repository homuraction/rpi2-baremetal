#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__

#include "stdint.h"

uint32_t get_and_clr_dispatch_cores(void);
void register_dispatch_cores(uint32_t cpuid);
void clr_save_flg(void);
void dispatch(void);
void save_context(uint32_t cpuid);
void context_switch(uint32_t cpuid);

#endif /* __DISPATCHER_H__ */
