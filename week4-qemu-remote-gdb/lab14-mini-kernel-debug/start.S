.equ UART0, 0x10000000

#ifndef SCENARIO_ID
#define SCENARIO_ID 1
#endif

.section .text
.globl _start
.globl kernel_entry
.globl expected_checkpoint
.globl early_boot_hang
.globl wrong_entry_target
.globl reset_trigger
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

#if SCENARIO_ID == 0
    call expected_checkpoint
#elif SCENARIO_ID == 1
    j early_boot_hang
#elif SCENARIO_ID == 2
    j wrong_entry_target
#elif SCENARIO_ID == 3
    j reset_trigger
#else
    j early_boot_hang
#endif
    ret

expected_checkpoint:
    la t0, boot_state
    li t1, 0x39141400
    sw t1, 0(t0)
    la a0, msg_good
    call uart_puts
    ret

early_boot_hang:
    la t0, boot_state
    li t1, 0x39141401
    sw t1, 0(t0)
    la a0, msg_hang
    call uart_puts
1:
    j 1b

wrong_entry_target:
    la t0, boot_state
    li t1, 0x39141402
    sw t1, 0(t0)
    la a0, msg_wrong_entry
    call uart_puts
    j halt_loop

reset_trigger:
    la t0, boot_state
    li t1, 0x39141403
    sw t1, 0(t0)
    la a0, msg_reset
    call uart_puts
    csrw mtvec, zero
    ecall
    j halt_loop

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
    .asciz "Lab 14 kernel_entry reached\n"
msg_good:
    .asciz "Scenario good: expected_checkpoint reached\n"
msg_hang:
    .asciz "Scenario hang: early_boot_hang is spinning\n"
msg_wrong_entry:
    .asciz "Scenario wrong-entry: wrong_entry_target reached\n"
msg_reset:
    .asciz "Scenario reset: mtvec=0 then ecall triggers trap-like behavior\n"

.section .data
boot_state:
    .word 0

.section .bss
.align 4
stack:
    .skip 4096
stack_top:
