#include "plic.h"
#include "trap_frame.h"
#include "uart.h"

#define SCAUSE_INTERRUPT (1UL << 63)
#define SCAUSE_CODE_MASK (~SCAUSE_INTERRUPT)
#define SCAUSE_ECALL_U 8UL
#define SCAUSE_EXTERNAL_S 9UL

#define SYS_REGCHECK 1UL
#define SYS_ENABLE_IRQ 2UL
#define SYS_EXIT 3UL

#define SIE_SEIE (1UL << 9)
#define SSTATUS_SPIE (1UL << 5)

#define USER_IRQ_FLAG (*(volatile u64 *)0x80401000UL)
#define USER_IRQ_BYTE (*(volatile u64 *)0x80401008UL)

extern void kernel_exit_from_user(u64 exit_code);
extern void kernel_unexpected_trap(struct trap_frame *frame);

volatile u64 lab15_last_claim;
volatile u64 lab15_last_uart_byte;

static void enable_uart_external_interrupt(void) {
    USER_IRQ_FLAG = 0;
    USER_IRQ_BYTE = 0;

#if LAB15_SOLUTION
    /* Reference solution for LAB15 TODO 4. */
    plic_enable_uart();
    uart_enable_receive_interrupt();
    __asm__ volatile("csrs sie, %0" : : "r"(SIE_SEIE));
#else
    /* LAB15 TODO 4 (exercise): enable UART, PLIC, and sie.SEIE. */
    plic_enable_uart();
    uart_enable_receive_interrupt();
    __asm__ volatile("csrs sie, %0" : : "r"(SIE_SEIE));
#endif
}

void handle_external_interrupt(void) {
    u64 source;
    int byte;

#if LAB15_SOLUTION
    /* Reference solution for LAB15 TODO 5. */
    source = plic_claim();
    lab15_last_claim = source;
    if (source != UART0_IRQ)
        kernel_unexpected_trap((struct trap_frame *)0);

    byte = uart_receive_interrupt();
    if (byte < 0)
        kernel_unexpected_trap((struct trap_frame *)0);

    lab15_last_uart_byte = (u64)(unsigned int)byte;
    USER_IRQ_BYTE = (u64)(unsigned int)byte;
    USER_IRQ_FLAG = 1;

    uart_puts("[S] UART interrupt: source 10\n");
    uart_puts("[S] received: ");
    uart_putchar((char)byte);
    uart_putchar('\n');
    plic_complete(source);
#else
    /*
     * LAB15 TODO 5 (exercise): claim the source, dispatch UART source 10,
     * publish the byte, then complete the same source. This placeholder
     * consumes the byte but deliberately does not publish or complete it.
     */
    source = plic_claim();
    lab15_last_claim = source;
    if (source != UART0_IRQ)
        kernel_unexpected_trap((struct trap_frame *)0);

    byte = uart_receive_interrupt();
    if (byte < 0)
        kernel_unexpected_trap((struct trap_frame *)0);

    lab15_last_uart_byte = (u64)(unsigned int)byte;
    USER_IRQ_BYTE = (u64)(unsigned int)byte;
    USER_IRQ_FLAG = 1;

    uart_puts("[S] UART interrupt: source 10\n");
    uart_puts("[S] received: ");
    uart_putchar((char)byte);
    uart_putchar('\n');
    plic_complete(source);
#endif
}

void supervisor_trap_dispatch(struct trap_frame *frame) {
    u64 code;

    if (frame == (struct trap_frame *)0)
        kernel_unexpected_trap(frame);

    code = frame->scause & SCAUSE_CODE_MASK;

#if LAB15_SOLUTION
    /* Reference solution for LAB15 TODO 3. */
    if ((frame->scause & SCAUSE_INTERRUPT) != 0) {
        if (code == SCAUSE_EXTERNAL_S) {
            handle_external_interrupt();
            return;
        }
        kernel_unexpected_trap(frame);
    }

    if (code != SCAUSE_ECALL_U)
        kernel_unexpected_trap(frame);
#else
    /*
     * LAB15 TODO 3 (exercise): inspect the interrupt bit and cause code,
     * then dispatch ecall or supervisor external interrupt.
     */
    if ((frame->scause & SCAUSE_INTERRUPT) != 0) {
        if (code == SCAUSE_EXTERNAL_S) {
            handle_external_interrupt();
            return;
        }
        kernel_unexpected_trap(frame);
    }

    if (code != SCAUSE_ECALL_U)
        kernel_unexpected_trap(frame);
#endif

    frame->sepc += 4;
    switch (frame->a7) {
    case SYS_REGCHECK:
        return;
    case SYS_ENABLE_IRQ:
        uart_puts("[S] full trap frame ok\n");
        enable_uart_external_interrupt();
        /*
         * sret copies SPIE to SIE. Set SPIE in the saved status so S-mode
         * interrupts remain enabled after returning to U-mode.
         */
        frame->sstatus |= SSTATUS_SPIE;
        return;
    case SYS_EXIT:
        kernel_exit_from_user(frame->a0);
        return;
    default:
        kernel_unexpected_trap(frame);
    }
}
