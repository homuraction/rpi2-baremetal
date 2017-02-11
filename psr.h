#include "stdint.h"

#ifndef __PSR_H__
#define __PSR_H__

#define PSR_USR 0x10
#define PSR_FIQ 0x11
#define PSR_IRQ 0x12
#define PSR_SVC 0x13
#define PSR_ABT 0x17
#define PSR_HYP 0x1A
#define PSR_UDF 0x1B
#define PSR_SYS 0x1F

#define PSR_M   0x1F
#define PSR_T   0x20
#define PSR_F   0x40
#define PSR_I   0x80
#define PSR_A  0x100
#define PSR_E  0x200

uint32_t get_cpsr(void);
uint32_t get_cpsr_m(uint32_t cpsr);

#endif /* __PSR_H__ */
