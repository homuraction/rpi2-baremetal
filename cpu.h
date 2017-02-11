#ifndef __CPU_H__
#define __CPU_H__

#include "stdint.h"

typedef struct usr_regs {
        uint32_t r[13]; // r0-r12
        void     *sp;   // stack pointer
        uint32_t lr;    // link register
        uint32_t pc;    // program counter
} usr_regs_t;

uint32_t get_prid(void);
uint32_t get_waking_cores(void);
void inc_waking_cores(void);
void spin_req(uint32_t *lock);
void spin_rel(uint32_t *lock);

#endif /* __CPU_H__ */
