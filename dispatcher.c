#include "dispatcher.h"
#include "stdint.h"
#include "multicore.h"

/*
uint32_t dispatch_cores = 0;
uint32_t dispatch_cores_lock = 0;
uint32_t save_flg[SMP_CORE_NUM] = {0};
uint32_t save_flg_lock = 0;

void dispatch(void)
{
        clr_save_flg();
        uint32_t cores = get_and_clr_dispatch_cores();

        uint32_t n_core;
        for (n_core=0x2; n_core < 0x10; n_core = n_core << 1) {
                if ((cores & n_core) > 0) {
                        if (n_core == 0x2) {
                                send_ipi(1, SAVE_CONTEXT);
                        } else if (n_core == 0x4) {
                                send_ipi(2, SAVE_CONTEXT);
                        } else if (n_core == 0x8) {
                                send_ipi(3, SAVE_CONTEXT);
                        }
                }
        }

        if (cores & 0x1 > 0)
                save_context(PCORE);
}

uint32_t get_and_clr_dispatch_cores(void)
{
        uint32_t cores;

        spin_req(&dispatch_cores_lock);

        cores = dispatch_cores;
        dispatch_cores = 0;

        spin_rel(&dispatch_cores_lock);

        return cores;
}

void register_dispatch_cores(uint32_t cpuid)
{
        spin_req(&dispatch_cores_lock);
        dispatch_cores = dispatch_cores || (1 << cpuid);
        spin_rel(&dispatch_cores_lock);
}

void context_switch(uint32_t n_core)
{
        save_context(n_core);
}

// save_context is called from IRQ mode
void save_context(uint32_t cpuid)
{
        // save context to current_tcb[n_core]
        save_flg[cpuid] = 1;
}

void clr_save_flg(void)
{
        uint32_t cpuid;
        for (cpuid=0; cpuid < SMP_CORE_NUM; cpuid++) {
                save_flg[cpuid] = 0;
        }
}
*/
