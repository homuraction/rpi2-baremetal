#include "init.h"
#include "mmu.h"
#include "boot.h"
#include "hwio.h"
#include "uart.h"
#include "interrupt.h"
#include "sysinfo.h"
#include "cpu.h"
#include "util.h"
#include "smp_kernel.h"
#include "multicore.h"
#include "experiment.h"

extern int main (void);

void sys_init_primary(void)
{
        // 0 clear .bss section data
        extern unsigned char __bss_start;
        extern unsigned char __bss_end;
        unsigned char *pBSS;
        for (pBSS = &__bss_start; pBSS < &__bss_end; pBSS++) {
                *pBSS = 0;
        }

        init_irq_handler();

        write32(DISABLE_IRQ1, 1<<29); // Disable interrupt from AUX
        mini_uart_init();             // Init Mini UART device
        write32(ENABLE_IRQ1,  1<<29); // Enable  interrupt from AUX

        _enable_interrupts();

        init_sysinfos();

        init_ttbs();                  // set up translation tables for each mode

        init_mmu();

        const uint32_t cpuid     = _get_prid();
        const uint32_t pm        = get_pm();

        start_mmu_no_lock(cpuid, pm);

        inc_waking_cores();

        init_smp_kernel();
}

void sys_init_secondary(void)
{
        _enable_interrupts();

        const uint32_t cpuid     = _get_prid();
        const uint32_t pm        = get_pm();

        busy_wait(2000000*cpuid);

        init_ttbs();                  // set up translation tables for each mode

        init_mmu();

        start_mmu_no_lock(cpuid, pm);

        write32(ARM_LOCAL_MAILBOX3_CLR0 + 0x10 * cpuid, 0);     // clear mailbox
        write32(ARM_LOCAL_MAILBOX_INT_CONTROL0 + 4 * cpuid, 1); // enable IPI on core n

        __asm__ volatile ("cpsie i");

        inc_waking_cores();

        init_smp_kernel();
}

void init_ttbs(void)
{
        uint32_t ttba;

        ttba = SYSCommonInfo->base_ttba;

        /* Set up Translation Table for each combination */
        uint32_t amp_pm = 0xF;
        uint32_t smp_pm[NUM_CPU-1] = { 0x0, 0x8, 0xC };
        uint32_t i;
        /*
         * (2 * NUM_CPU - 1) indicates that a number of the combination of
         * processing mode.
         */
        for (i=0; i < (2 * NUM_CPU) - 1; i++, ttba += SECTION_TBL_SIZE) {
                if (i < NUM_CPU) {
                        amp_tbl_setup(ttba,
                                      get_kernel_ram_area(i, amp_pm),
                                      get_user_ram_area(i, amp_pm),
                                      get_kernel_heap_area(i, amp_pm),
                                      SP_BASE + (SECTION_SIZE*i));
                } else {
                        smp_tbl_setup(ttba,
                                      get_kernel_ram_area(PCore, smp_pm[i-NUM_CPU]),
                                      get_user_ram_area(PCore, smp_pm[i-NUM_CPU]),
                                      get_kernel_heap_area(PCore, smp_pm[i-NUM_CPU]),
                                      SP_BASE,
                                      NUM_CPU - (i - NUM_CPU));
                }
        }
}

void init_smp_kernel(void)
{
        const uint32_t cpuid   = _get_prid();
        const uint32_t pm      = get_pm();
        const uint32_t prev_pm = get_prev_pm();

        if (cpuid == PCore) {
                smp_queue_lock = 0;

                const SysStructInfo *ssi =
                        get_sys_struct_info(cpuid, pm);

                if (smp_tsk_alc_map != NULL) {
                        kfree(smp_tsk_alc_map);
                }

                smp_tsk_alc_map =
                        (task_map_t *)kmalloc(sizeof(task_map_t) *
                                SYSStructTop[ALL_SMP_MODE]->max_num_task_space);

                start_secondary_core();

                busy_wait(10000000);

                while(1) {
                        if (get_waking_cores() >= 4) break;
                }

                output_all_pm_access_policy();
        } else {
                __asm__ volatile("cpsie i");
                busy_wait(10000*cpuid);
                if (cpuid == 1)
                        inc_waking_cores();
        }
}

void init_amp_kernel(void)
{
}
