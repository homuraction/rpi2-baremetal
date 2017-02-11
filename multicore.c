#include "multicore.h"
#include "hwio.h"
#include "stdint.h"
#include "boot.h"
#include "uart.h"
#include "cpu.h"
#include "experiment.h"
#include "mmu.h"

uint32_t start_secondary_core (void)
{
        uint32_t n_routing = IO_READ(ARM_LOCAL_GPU_INT_ROUTING);

        n_routing &= ~0x0C;
        n_routing |= 1 << 2;

        IO_WRITE(ARM_LOCAL_GPU_INT_ROUTING, n_routing);
        IO_WRITE(ARM_LOCAL_MAILBOX_INT_CONTROL0, 1); // enable IPI on core 0

        uint32_t n_core;
        for (n_core = 1; n_core < CORES; n_core++)
        {
                uint32_t n_mb_clr = ARM_LOCAL_MAILBOX3_CLR0 + 0x10 * n_core;

                DSB();

                unsigned n_timeout = 0;
                while (read32(n_mb_clr) != 0)
                {
                        register_value_puts("n_timeout: ", n_timeout);
                        if (++n_timeout > 1000000)
                        {
                                uart_puts("CPU core ");
                                hexstring(n_core);
                                uart_puts("does not respond\n");

                                return FALSE;
                        }
                }

                write32(ARM_LOCAL_MAILBOX3_SET0 + 0x10 * n_core, (uint32_t)&_secondary_start);

                n_timeout = 0;
                while (read32(n_mb_clr) != 0)
                {
                        if (++n_timeout > 1000000)
                        {
                                uart_puts("CPU core ");
                                hexstring(n_core);
                                uart_puts("did not start\n");

                                return FALSE;
                        }
                }
                volatile uint32_t cnt = 1000;
                while(cnt) {
                        cnt--;
                }
                busy_wait(1000000);
        }

        return TRUE;
}

void ipi_handler (uint32_t n_core, uint32_t n_ipi)
{
        if (n_ipi == 0x1)
        {
                register_value_puts("Hello from CPU core ", _get_prid());
        } else if (n_ipi == 0x2) {
                output_access_policy();
        }
        /*
        if (n_core == PCORE && n_ipi == DISPATCH) {
                //dispatch();
        }
        if (n_core != PCORE && is_smp(n_core) && n_ipi == CONTEXT_SWITCH) {
                //context_switch(n_core);
        }
        */
}

void send_ipi (uint32_t n_core, uint32_t n_ipi)
{
        write32(ARM_LOCAL_MAILBOX0_SET0 + 0x10 * n_core, 1 << n_ipi);
}

uint32_t local_int_handler (void)
{
        uint32_t n_core = _get_prid();
        if (!(read32(ARM_LOCAL_IRQ_PENDING0 + 4 * n_core) & 0x10))
        {
                return FALSE;
        }

        uint32_t n_mb_clr = ARM_LOCAL_MAILBOX0_CLR0 + 0x10 * n_core;
        uint32_t n_ipi_mask = read32(n_mb_clr);
        if (n_ipi_mask == 0)
        {
                return FALSE;
        }

        uint32_t n_ipi;
        for (n_ipi = 0; !(n_ipi_mask & 1); n_ipi++)
        {
                n_ipi_mask >>= 1;
        }

        write32 (n_mb_clr, 1 << n_ipi);
        DSB();

        ipi_handler(n_core, n_ipi);

        return TRUE;
}
