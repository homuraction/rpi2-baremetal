#include "hwio.h"

#ifndef __UART_H__
#define __UART_H__

uint32_t uart_lock;

volatile uint32_t uart_lcr ( void );
volatile uint32_t uart_recv ( void );
unsigned int uart_check ( void );
void uart_send ( char c );
void uart_puts( char* c );
void hexstring ( uint32_t d );
void register_value_puts(char *str, uint32_t hex);
void uart_flush ( void );
void mini_uart_init ( void );
void timer_init ( void );
uint32_t timer_tick ( void );

#endif /* __UART_H__ */
