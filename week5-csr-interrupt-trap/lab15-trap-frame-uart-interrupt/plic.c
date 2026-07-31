#include "plic.h"
#include "uart.h"

/*
 * QEMU virt maps the PLIC MMIO region at 0x0c000000.
 * Loads and stores to this region access PLIC hardware registers, not
 * ordinary RAM. The offsets below come from the PLIC register layout and
 * the hart 0 S-mode context used by this lab.
 */
#define PLIC_BASE 0x0c000000UL
#define PLIC_PRIORITY(source) \
    (*(volatile unsigned int *)(PLIC_BASE + 4UL * (source)))
#define PLIC_S_ENABLE \
    (*(volatile unsigned int *)(PLIC_BASE + 0x2080UL))
#define PLIC_S_THRESHOLD \
    (*(volatile unsigned int *)(PLIC_BASE + 0x201000UL))
#define PLIC_S_CLAIM_COMPLETE \
    (*(volatile unsigned int *)(PLIC_BASE + 0x201004UL))

/*
 * ECE391 expected skill:
 *
 * Given the MMIO macros above, students should be able to write these three
 * assignments:
 *   1. give UART source 10 a non-zero priority;
 *   2. enable source 10 for hart 0's S-mode context;
 *   3. set a threshold lower than the UART priority.
 *
 * Lab 15 provides the implementation so its required TODOs can stay focused
 * on the trap frame and interrupt-dispatch path.
 */
void plic_enable_uart(void) {
    unsigned int uart_source_mask;
    unsigned int enabled_sources;

    PLIC_PRIORITY(UART0_IRQ) = 1;

    /* UART0_IRQ is source 10, so this creates a value with only bit 10 set. */
    uart_source_mask = 1U << UART0_IRQ;

    /* Preserve sources that were already enabled, then also enable UART. */
    enabled_sources = PLIC_S_ENABLE;
    enabled_sources = enabled_sources | uart_source_mask;
    PLIC_S_ENABLE = enabled_sources;

    PLIC_S_THRESHOLD = 0;
}

/*
 * ECE391 expected skill:
 *
 * Reading the claim/complete register returns the highest-priority pending
 * source ID for this context. Students should be able to write this one MMIO
 * read when the register macro is provided.
 */
plic_u64 plic_claim(void) {
    return (plic_u64)PLIC_S_CLAIM_COMPLETE;
}

/*
 * ECE391 expected skill:
 *
 * After servicing the device, write the same source ID returned by claim back
 * to the claim/complete register. Students should be able to write this one
 * MMIO store when the register macro is provided.
 */
void plic_complete(plic_u64 source) {
    PLIC_S_CLAIM_COMPLETE = (unsigned int)source;
}
