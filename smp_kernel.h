#ifndef __SMP_KERNEL_H__
#define __SMP_KERNEL_H__

#include "init.h"
#include "mmu.h"
#include "boot.h"
#include "hwio.h"
#include "uart.h"
#include "interrupt.h"
#include "sysinfo.h"
#include "cpu.h"
#include "util.h"
#include "task.h"

#define SMP_MAX_NUM_TASK 256
#define SMP_MAX_PRI      128

task_map_t *smp_tsk_alc_map;
TCB        *smp_tcb_head[SMP_MAX_PRI];
uint32_t    smp_queue_lock;

void smp_kernel(void);

#endif /* __SMP_KERNEL_H__ */
