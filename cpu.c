#include "cpu.h"
#include "boot.h"
#include "sysinfo.h"
#include "uart.h"

uint32_t get_prid(void)
{
        uint32_t prid;
        __asm__ volatile("svc #10\n"
                         "and %0,r0,#0x3"
                          : "=r"(prid)
                        );
        return prid;
}

uint32_t get_waking_cores(void) {
        uint32_t waking_cores;

        spin_req((uint32_t *)&(SYSCommonInfo->lock));
        waking_cores = SYSCommonInfo->waking_cores;
        spin_rel((uint32_t *)&(SYSCommonInfo->lock));

        return waking_cores;
}

void inc_waking_cores(void)
{
        spin_req(&(SYSCommonInfo->lock));
        SYSCommonInfo->waking_cores += 1;
        spin_rel((uint32_t *)&(SYSCommonInfo->lock));
}

void spin_req(uint32_t *lock)
{
        _spin_req(lock);
        uint32_t prid = _get_prid();
        if (prid != 0)
                register_value_puts("get lock from: ", prid);
        /*
        while (1) {
                if (*lock == 0) {
                        *lock = 1;
                        //uart_puts("get lock\n");
                        return;
                }
        }
        */
}

void spin_rel(uint32_t *lock)
{
        _spin_rel(lock);
        uint32_t prid = _get_prid();
        if (prid != 0)
                register_value_puts("release lock from: ", prid);
        /*
        while (1) {
                if (*lock == 1) {
                        *lock = 0;
                        //uart_puts("release lock\n");
                        return;
                }
        }
        */
}
