#ifndef LAB15_UART_H
#define LAB15_UART_H

#define UART0_IRQ 10UL

void uart_putchar(char value);
void uart_puts(const char *text);
void uart_enable_receive_interrupt(void);
int uart_receive_interrupt(void);

#endif

