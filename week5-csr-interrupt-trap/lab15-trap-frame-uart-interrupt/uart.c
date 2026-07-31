#include "uart.h"

#define UART0_BASE 0x10000000UL
#define UART_RBR (*(volatile unsigned char *)(UART0_BASE + 0))
#define UART_THR (*(volatile unsigned char *)(UART0_BASE + 0))
#define UART_IER (*(volatile unsigned char *)(UART0_BASE + 1))
#define UART_LSR (*(volatile unsigned char *)(UART0_BASE + 5))
#define UART_IER_DRIE 0x01
#define UART_LSR_DR 0x01
#define UART_LSR_THRE 0x20

void uart_putchar(char value) {
    while ((UART_LSR & UART_LSR_THRE) == 0)
        ;
    UART_THR = (unsigned char)value;
}

void uart_puts(const char *text) {
    while (*text != '\0')
        uart_putchar(*text++);
}

void uart_enable_receive_interrupt(void) {
    UART_IER |= UART_IER_DRIE;
}

int uart_receive_interrupt(void) {
    if ((UART_LSR & UART_LSR_DR) == 0)
        return -1;
    return (int)UART_RBR;
}

