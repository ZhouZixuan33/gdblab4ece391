/*
 * Bare-metal entry for Lab 12.
 *
 * QEMU reaches _start in M-mode. There is no hosted C runtime, so this file
 * supplies a stack before calling the C entry routine.
 */

.section .text.start
.globl _start
.globl halt_loop
.extern kernel_entry

_start:
    la sp, stack_top
    call kernel_entry

halt_loop:
    wfi
    j halt_loop

.section .bss
.balign 16
stack:
    .skip 4096
.globl stack_top
stack_top:
