#include "stdint.h"

#ifndef __BOOT_H__
#define __BOOT_H__

void _enable_interrupts(void);
uint32_t _get_cpsr(void);
uint32_t _get_prid(void);
void _send_sgi(uint32_t a, uint32_t b, uint32_t c);
uint32_t _get_mpidr(void);
uint32_t _get_num_cpus(void);
uint32_t _start1(void);
uint32_t _start2(void);
uint32_t _start3(void);
uint32_t _is_smp(void);
void _set_smp(void);
uint32_t _get_sctlr(void);
uint32_t _get_hsctlr(void);
uint32_t _get_hvbadr(void);
uint32_t _get_vbadr(void);
uint32_t _get_stack_pointer(void);
uint32_t read32(uint32_t reg);
void write32(uint32_t reg, uint32_t value);
uint32_t _secondary_start(void);
void _spin_req(uint32_t *adr);
void _spin_rel(uint32_t *adr);
uint32_t _get_dacr(void);
uint32_t _get_contextidr(void);

void invalidate_data_cache_l10_only(void);

#endif /* __BOOT_H__ */
