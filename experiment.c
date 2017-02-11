#include "stdint.h"
#include "experiment.h"
#include "uart.h"
#include "boot.h"
#include "sysinfo.h"
#include "multicore.h"

static uint32_t put_cnt;
static uint32_t exp_lock;

void check_address(const uint32_t address)
{
        hexstring(address);
        uart_puts(": ");
        hexstring(read32(address));
        uart_puts("\n");
}

void output_access_policy(void)
{
        spin_req(&exp_lock);
        uart_puts("========================================================\n");
        register_value_puts("CPUID: ", _get_prid());
        uint32_t i;
        for (i = 0x00000000; i < 0x43000000; i += 0x00100000) {
                check_address(i);
        }
        uart_puts("========================================================\n");
        spin_rel(&exp_lock);
}

void output_all_pm_access_policy(void)
{
        uint32_t i;
        for (i=0; i < NUM_CPU; i++) {
                if (i == _get_prid())
                        output_access_policy();
                else if (i > 1)
                        send_ipi(i, 0x2);
        }
}
