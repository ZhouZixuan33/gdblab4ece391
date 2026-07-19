/*
 * Tiny RISC-V entry point for Lab 11.
 * QEMU loads kernel.elf, starts at _start, and GDB reads the same ELF
 * for symbols.
 */

.section .text.start
.globl _start
.globl halt_loop
.extern kernel_entry
.extern g_main_return

_start:
    la sp, stack_top
    call kernel_entry

    la t0, g_main_return
    sd a0, 0(t0)

halt_loop:
    wfi
    j halt_loop

.section .bss
.balign 16
stack:
    .skip 4096
.globl stack_top
stack_top:
