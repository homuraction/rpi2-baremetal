#include "hwio.h"
#include "sysinfo.h"
#include "uart.h"
#include "boot.h"
#include "psr.h"
#include "mmu.h"
#include "interrupt.h"
#include "cpu.h"
#include "experiment.h"

volatile int busy_wait(int32_t count) {
        volatile int32_t cnt = count;
        while(cnt > 0) {
                cnt--;
        }
        return 0;
}

typedef int32_t BOOL;

#define INITIALIZING 0
#define INITIALIZED  1

uint32_t __init_status = INITIALIZING;

int main(void) {
        //register_value_puts("TTBA: ", get_ttba0());

        /*
        uint32_t cpuid = _get_prid();

        uint32_t i, j, pm;
        SysStructInfo *ssi;
        for (i=0; i<2*NUM_CPU-1; i++) {
                if (i < NUM_CPU) {
                        if (i != PCore) continue;
                        register_value_puts("ACCESS CHECK FROM AMP No: ", i);
                } else {
                        register_value_puts("ACCESS CHECK FROM SMP No: ", i-NUM_CPU);
                }

                register_value_puts("SYSStructInfo INDEX: ", i);

                if (i == 4) {
                        change_pm(0x0);
                } else if (i == 5) {
                        change_pm(0x8);
                } else if (i == 6) {
                        change_pm(0xC);
                }

                pm = get_pm();
                register_value_puts("pm: ", pm);
                ssi = get_sys_struct_info(cpuid, pm);
                uart_puts("KEMONO FRIENDS\n");
                start_mmu(ssi->ttba, MMU_ENABLE | ICACHE_ENABLE | DCACHE_ENABLE,
                                cpuid, pm);

                for (j = 0x00000000; j < 0x3F000000; j += 0x00100000) {
                        check_address(j);
                }

                uart_puts("================================================\n");
        }
        */
        //output_access_policy();
        while(1) {

                busy_wait(1000000);
                //if (get_waking_cores() > 3) break;
        }

        /*
        volatile uint32_t prno;
        for (prno=1; prno < 4; prno++) {
                send_ipi(prno, 1);
                busy_wait(10000);
        }
        */

        //while (SYSCommonInfo->waking_cores < 7) {};

        return 0;
}

int other(void) {
        uart_puts("other\n");

        uint32_t prid = _get_prid();
        char str[6] = { 'C', 'P', 'U', '0'+prid, '\n', '\0' };

        //uint32_t local_init_status;
        while(1) {
                //spin_req(&(init_status_lock));
                //local_init_status = __init_status;
                //spin_rel(&(init_status_lock));
                if (__init_status == INITIALIZED) break;
                register_value_puts("init status: ", __init_status);
                busy_wait(10000);
        }

        while(1) {
                uart_puts("FooooooooooooooHAHAHAHAHAHAHA!!!!!!!!\n");
        }

        /*
        if (prid == 1)
        {
                sync_ttba1_change(0x10000);
                start_mmu(SYSStructTop[0]->tlb_base_address,
                        MMU_ENABLE | ICACHE_ENABLE | DCACHE_ENABLE);
                invalidate_tlbs();
                register_value_puts("CORE1 TTBA1: ", get_ttba1());
                register_value_puts("CORE1 DD: ", IO_READ(0x00345678));
        }
        */

        /*
        if (prid == 2) {
                mmu_section(0x00300000, 0x0BB00000, 0x0000,0);
        }
        */

        write32(ARM_LOCAL_MAILBOX3_CLR0 + 0x10 * prid, 0);     // clear mailbox
        write32(ARM_LOCAL_MAILBOX_INT_CONTROL0 + 4 * prid, 1); // enable IPI on core n
        //write32(ARM_LOCAL_MAILBOX_INT_CONTROL0 + 4 * prid, 0xF); // enable IPI on core n
        __asm__ volatile("cpsie i");                           // enable interrupt

        busy_wait(100000*prid);
        //uart_puts(str);
        //register_value_puts("TTBA0: ", get_ttba0());
        busy_wait(1000);
        //register_value_puts("TTBA1: ", get_ttba1());
        //uart_send('\n');

        inc_waking_cores();

        uart_puts("over inc_waking_cores()\n");

        while(1) {
                busy_wait(10000);
        }

        return 0;
}
