/*
 * Tiny RISC-V target for Lab 11.
 * QEMU loads this ELF on the virt machine; GDB reads the same ELF for symbols.
 */

.equ UART0, 0x10000000

.section .text
.globl _start
.globl kernel_entry
.globl hello_checkpoint
.globl halt_loop

_start:
    la sp, stack_top
    call kernel_entry

halt_loop:
    wfi
    j halt_loop

kernel_entry:
    la a0, msg_entry
    call uart_puts
    call hello_checkpoint
    la a0, msg_done
    call uart_puts
    ret

hello_checkpoint:
    la t0, checkpoint_value
    li t1, 0x39141100
    sw t1, 0(t0)
    la a0, msg_checkpoint
    call uart_puts
    ret

uart_puts:
    mv t0, a0
1:
    lbu t1, 0(t0)
    beqz t1, 3f
2:
    li t2, UART0 + 5
    lbu t3, 0(t2)
    andi t3, t3, 0x20
    beqz t3, 2b
    li t2, UART0
    sb t1, 0(t2)
    addi t0, t0, 1
    j 1b
3:
    ret

.section .rodata
msg_entry:
    .asciz "Lab 11 kernel_entry reached\n"
msg_checkpoint:
    .asciz "Lab 11 hello_checkpoint reached\n"
msg_done:
    .asciz "Lab 11 done; CPU will wait in halt_loop\n"

.section .data
checkpoint_value:
    .word 0

.section .bss
.align 4
stack:
    .skip 4096
stack_top:
