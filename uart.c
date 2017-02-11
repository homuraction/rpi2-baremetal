#include "hwio.h"
#include "uart.h"
#include "cpu.h"

extern void dummy ( unsigned int );

// GPIO14 TXD0 and TXD1
// GPIO15 RXD0 and RXD1

volatile uint32_t uart_lcr ( void )
{
        return (IO_READ(MU_LSR));
}

volatile uint32_t uart_recv ( void )
{
        while(1) {
                if (IO_READ(MU_LSR) & MU_LSR_RX_READY) break;
        }
        return (IO_READ(MU_IO) & MU_IO_RX_DATA);
}

unsigned int uart_check ( void )
{
        if (IO_READ(MU_LSR) & MU_LSR_RX_READY) return(1);
        return (0);
}

void uart_send ( char c )
{
        while (1) {
                if (IO_READ(MU_LSR) & MU_LSR_TX_EMPTY) break;
        }
        if (c == '\n' || c == '\r') {
                IO_WRITE(MU_IO, '\n');
                while (!(IO_READ(MU_LSR) & MU_LSR_TX_EMPTY)) {}
                IO_WRITE(MU_IO, '\r');
        } else
                IO_WRITE(MU_IO, c);
}

void uart_puts( char* c )
{
        if (is_mmu_enabled()) {
                spin_req(&uart_lock);
                while ( *c != '\0' ) {
                        uart_send(*c);
                        c++;
                }
                spin_rel(&uart_lock);
        } else {
                while ( *c != '\0' ) {
                        uart_send(*c);
                        c++;
                }
        }
}

void hexstring ( uint32_t d )
{
        uint32_t ls, rs;
        uint32_t rb;
        uint32_t rc;

        rb=32;
        if (is_mmu_enabled()) {
                spin_req(&uart_lock);
                while(1) {
                        rb -= 4;
                        rc  = (d >> rb) & 0xF;
                        if (rc > 9)
                                rc += 0x37;
                        else
                                rc += 0x30;
                        uart_send(rc);
                        if (rb == 0) break;
                }
                uart_send(0x20);
                spin_rel(&uart_lock);
        } else {
                while(1) {
                        rb -= 4;
                        rc  = (d >> rb) & 0xF;
                        if (rc > 9)
                                rc += 0x37;
                        else
                                rc += 0x30;
                        uart_send(rc);
                        if (rb == 0) break;
                }
                uart_send(0x20);
        }
}

void register_value_puts(char *str, uint32_t hex)
{
        uart_puts(str);
        hexstring(hex);
        uart_send('\n');
}

void uart_flush ( void )
{
        while(1) {
                if ((IO_READ(MU_LSR) & 0x100) == 0) break;
        }
}

void mini_uart_init ( void )
{
        uint32_t ra, ac;

        IO_WRITE(AUX_ENABLES, 0x01);  // Enable mini UART
        IO_WRITE(MU_IER,      0x00);  // Disable transmit/receiver interrupts
        IO_WRITE(MU_CNTL,     0x00);  // Disable Transmitter and Receiver
        IO_WRITE(MU_LCR,      0x03);  // UART works 8-bit mode
        IO_WRITE(MU_MCR,      0x00);  // UART1_RTS line is high
        IO_WRITE(MU_IER,      0x05);  // Enable receive interrupt
        IO_WRITE(MU_IIR,      0xC6);  // Clear the TX/RX FIFO and enable them
        IO_WRITE(MU_BAUD,      270);  // SYS_FREQ / (8 * 115200) - 1 = 270

        // TX and RX take alternate function 5
        ra = IO_READ(GPIO_SEL1);      // Read GPFSEL1
        ra &= ~(7 << 12);             // XXXX XXXX XXXX XXXX X000 XXXX XXXX XXXX
        ra |=  (2 << 12);             // XXXX XXXX XXXX XXXX X010 XXXX XXXX XXXX
        ra &= ~(7 << 15);             // XXXX XXXX XXXX XX00 0010 XXXX XXXX XXXX
        ra |=  (2 << 15);             // XXXX XXXX XXXX XX01 0010 XXXX XXXX XXXX
        IO_WRITE(GPIO_SEL1, ra);      // Write (update) GPFSEL1

        IO_WRITE(GPIO_GPPUD, 0);      // Disable Pull-up/down
        for (ra=0; ra<150; ra++)      // Wait 150 cycles
                dummy(ra);

        ac = (1 << 14) | (1 << 15);   // Position of Assert Clock
        IO_WRITE(GPIO_GPPUDCLK0, ac); // Assert Clock on line TX and RX
        for (ra=0; ra<150; ra++)
                dummy(ra);

        IO_WRITE(GPIO_GPPUDCLK0, 0);  // Remove Clock on line TX and RX

        IO_WRITE(MU_CNTL, 0x3);       // Enable Transmitter and Receiver

        uart_lock = 0;
}

void timer_init ( void )
{
        // 0xF9+1 = 250
        // 250MHz/250 = 1MHz
        IO_WRITE(ARM_TIMER_CTL, 0x00F90000);
        IO_WRITE(ARM_TIMER_CTL, 0x00F90200);
}

uint32_t timer_tick ( void )
{
        return (IO_READ(ARM_TIMER_CNT));
}
