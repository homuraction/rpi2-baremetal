#include "hwio.h"
#include "uart.h"
#include "boot.h"
#include "psr.h"
#include "cpu.h"
#include "interrupt.h"
#include "memory.h"
#include "multicore.h"
#include "mmu.h"

extern void clear_uart_interrupt( void );
extern void uart_puts(char* c);
extern void uart_send(char c);
extern void hexstring(uint32_t d);

#define RXBUFMASK 0xFFF
volatile uint8_t  rxbuffer[RXBUFMASK+1];
volatile uint32_t rxhead;
volatile uint32_t rxtail;

typedef void TIRQHandler (void *p_param);
static TIRQHandler *m_ap_irq_handler[IRQ_LINES];
static void *m_p_param[IRQ_LINES];

void init_irq_handler(void) {
        m_ap_irq_handler[0x1d] = aux_int_handler;
}

static __inline__ void _disint( void )
{
        uint32_t imask;
        asm volatile("  mrs %0,     cpsr     \n"
                     "  orr %0,     %0,   %1 \n"
                     "  msr cpsr_c, %0         "
                        :
                        : "r"(imask),"i"(PSR_I|PSR_F)
                        );
}

static __inline__ void _enaint( void )
{
        uint32_t imask;
        asm volatile("  mrs %0,     cpsr     \n"
                     "  bic %0,     %0,   %1 \n"
                     "  msr cpsr_c, %0         "
                        :
                        : "r"(imask),"i"(PSR_I)
                        );
}

void enable_irq (uint32_t n_irq)
{
        DSB();

        write32(ARM_IC_IRQS_ENABLE (n_irq), ARM_IRQ_MASK(n_irq));

        DMB();
}

void disable_irq (uint32_t n_irq)
{
        DSB();

        write32 (ARM_IC_IRQS_DISABLE (n_irq), ARM_IRQ_MASK (n_irq));

        DMB();
}

void send_irq(uint32_t prid)
{
        uint32_t irq_adr = 0x18;
        uint32_t mbx_irq = 0x4000008C + (prid * 0x10);
        __asm__ volatile("str %0,[%1]" :: "r"(irq_adr),"r"(mbx_irq));
}

uint32_t call_irq_handler(uint32_t n_irq)
{
        TIRQHandler *p_handler = m_ap_irq_handler[n_irq];
        if (p_handler != 0) {
                (*p_handler)(m_p_param[n_irq]);

                return TRUE;
        } else {
                uart_puts("In call_irq_handler\n");
                register_value_puts("n_irq: ", n_irq);
                register_value_puts("m_ap_irq_handler: ", (uint32_t)m_ap_irq_handler);
                disable_irq (n_irq);
        }

        return FALSE;
}

void _interrupt_handler(void)
{
        //uart_puts("In _interrupt_handler\n");
        if (local_int_handler())
                return;

        //uart_puts("local_int_handler() returned false\n");

        uint32_t pending[3];
        pending[0] = read32(IRQ_PEND1);
        pending[1] = read32(IRQ_PEND2);
        pending[2] = read32(IRQ_BASIC_PEND) & 0xFF;

        //register_value_puts("pending0: ", pending[0]);
        //register_value_puts("pending1: ", pending[1]);
        //register_value_puts("pending2: ", pending[2]);

        uint32_t n_reg;
        for (n_reg = 0; n_reg < 3; n_reg++)
        {
                uint32_t n_pending = pending[n_reg];
                if (n_pending != 0)
                {
                        uint32_t n_irq = n_reg * 32;

                        do
                        {
                                if ( (n_pending & 1) && call_irq_handler(n_irq))
                                        return;
                                n_pending >>= 1;
                                n_irq++;
                        }
                        while (n_pending != 0);
                }
        }
}

void interrupt_handler(void) {
        DSB();  // exit from interrupted peripheral

        _interrupt_handler();

        DMB(); // continuing with interrupted peripheral
}

//void __attribute__((interrupt("IRQ"))) interrupt_vector ( void )
void aux_int_handler ( void )
{
        //_disint();
        uint32_t rb, rc;

        // an interrupt has occured, find out why
        while (1) { // resolve all interrupts to uart
                rb = IO_READ(MU_IIR);
                if((rb&1)==1) break; // no more interrupts
                if((rb&6)==4) {
                        while(!(IO_READ(MU_LSR) & 0x1))
                                ;
                        // receiver holds a valid byte
                        rc = IO_READ(MU_IO);
                        uart_send(rc);
                }
        }
        //hexstring(get_prid());uart_send('\n');
        //register_value_puts("IRQ SP: ", _get_stack_pointer());
        //_enaint();
}

/**
    @brief The Reset vector interrupt handler

    This can never be called, since an ARM core reset would also reset the
    GPU and therefore cause the GPU to start running code again until
    the ARM is handed control at the end of boot loading
*/
void __attribute__((interrupt("ABORT"))) reset_vector(void)
{
        _disint();
        uart_puts("Reset Vector ABORT!!\n");
        _enaint();
}

/**
    @brief The undefined instruction interrupt handler

    If an undefined intstruction is encountered, the CPU will start
    executing this function. Just trap here as a debug solution.
*/
void __attribute__((interrupt("UNDEF"))) undefined_instruction_vector(void)
{
    //while( 1 )
    //{
        /* Do Nothing! */
        _disint();
        uart_puts("Undefine Instruction!!\n");
        _enaint();
    //}
}


/**
    @brief The supervisor call interrupt handler

    The CPU will start executing this function. Just trap here as a debug
    solution.
*/
//void __attribute__((interrupt("SWI"))) software_interrupt_vector(uint32_t no)
uint32_t software_interrupt_handler(uint32_t no)
{
    /* Do Nothing! */
    //register_value_puts("SWI: ", _get_stack_pointer());
    register_value_puts("SWI: ", no);
    register_value_puts("SWI CORE: ", _get_prid());
    register_value_puts("StackPointer: ", _get_stack_pointer());
    return _get_prid();
}


/**
    @brief The prefetch abort interrupt handler

    The CPU will start executing this function. Just trap here as a debug
    solution.
*/
void /*__attribute__((interrupt("ABORT")))*/ prefetch_abort_vector(void)
{
        _disint();
        uart_puts("PREFETCH ABORT\n");
        _enaint();
}


/**
    @brief The Data Abort interrupt handler

    The CPU will start executing this function. Just trap here as a debug
    solution.
*/
void /*__attribute__((interrupt("ABORT")))*/ data_abort_vector(uint32_t address, uint32_t status)
{
        //_disint();
#define DABT_EXT_MASK 0x1000
#define DABT_WNR_MASK 0x8000
#define DABT_FS_MASK  0x40F
        hexstring(address);
        uart_puts(": ABORT caused by");
        if (status & DABT_EXT_MASK) {
                uart_puts(" ::external abort");
        }
        if (status & DABT_WNR_MASK) {
                // If fault by CP15 cache maintenance operation such a translate
                // VA to PA, this is always called.
                uart_puts(" ::write access");
        } else {
                uart_puts(" ::read access");
        }
        uint32_t fs = (status & DABT_FS_MASK);
        if (fs == 0x1) {
                uart_puts(" ::alignment fault");
        } else if (fs == 0x4) {
                uart_puts(" ::instruction cache maitenance fault");
        } else if (fs == 0xC) {
                uart_puts(" ::translation table walk sync external abort L1");
        } else if (fs == 0xE) {
                uart_puts(" ::translation table walk sync external abort L2");
        } else if (fs == 0x40C) {
                uart_puts(" ::translation table walk sync pality error L1");
        } else if (fs == 0x40E) {
                uart_puts(" ::translation table walk sync pality error L2");
        } else if (fs == 0x5) {
                uart_puts(" ::translation fault(MMU Fault) in section");
        } else if (fs == 0x7) {
                uart_puts(" ::translation fault(MMU Fault) in page");
        } else if (fs == 0x3) {
                uart_puts(" ::access flag fault(MMU Fault) in section");
        } else if (fs == 0x6) {
                uart_puts(" ::access flag fault(MMU Fault) in page");
        } else if (fs == 0x9) {
                uart_puts(" ::domain fault(MMU Fault) in section");
        } else if (fs == 0xB) {
                uart_puts(" ::domain fault(MMU Fault) in page");
        } else if (fs == 0xD) {
                uart_puts(" ::access policy fault(MMU Fault) in section");
        } else if (fs == 0xF) {
                uart_puts(" ::access policy fault(MMU Fault) in page");
        }
        uart_puts("\n");
        //_enaint();
}

void __attribute__((interrupt("FIQ"))) fast_interrupt_vector(void)
{

}
