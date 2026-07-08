.equ UART0, 0x10000000

#ifndef SCENARIO_ID
#define SCENARIO_ID 1
#endif

.section .text
.globl _start
.globl kernel_entry
.globl scenario_good
.globl scenario_bad_pointer
.globl bad_pointer_fault_site
.globl scenario_bad_jump
.globl bad_jump_source
.globl unexpected_landing
.globl scenario_illegal_instruction
.globl illegal_instruction_site
.globl halt_loop

_start:
    la sp, stack_top
    call kernel_entry

halt_loop:
    wfi
    j halt_loop

kernel_entry:
    la a0, msg_start
    call uart_puts

#if SCENARIO_ID == 0
    call scenario_good
#elif SCENARIO_ID == 1
    call scenario_bad_pointer
#elif SCENARIO_ID == 2
    call scenario_bad_jump
#elif SCENARIO_ID == 3
    call scenario_illegal_instruction
#else
    call scenario_bad_pointer
#endif
    ret

scenario_good:
    la a0, msg_good
    call uart_puts
    ret

scenario_bad_pointer:
    la a0, msg_bad_pointer
    call uart_puts
    li a0, 0xdeadbeef
bad_pointer_fault_site:
    ld a1, 0(a0)
    la a0, msg_pointer_continued
    call uart_puts
    .word 0xffffffff

scenario_bad_jump:
    la a0, msg_bad_jump
    call uart_puts
    la t0, unexpected_landing
bad_jump_source:
    jr t0

unexpected_landing:
    la a0, msg_unexpected_landing
    call uart_puts
    ret

scenario_illegal_instruction:
    la a0, msg_illegal
    call uart_puts
illegal_instruction_site:
    .word 0xffffffff
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
msg_start:
    .asciz "Lab 13 kernel_entry reached\n"
msg_good:
    .asciz "Scenario good: no exception-style failure\n"
msg_bad_pointer:
    .asciz "Scenario bad-pointer: a0 will hold 0xdeadbeef before the load\n"
msg_pointer_continued:
    .asciz "The bad pointer load returned; illegal instruction will force a trap\n"
msg_bad_jump:
    .asciz "Scenario bad-jump: control will jump through t0\n"
msg_unexpected_landing:
    .asciz "Unexpected landing reached; inspect pc and the jump source\n"
msg_illegal:
    .asciz "Scenario illegal-instruction: next word is intentionally illegal\n"

.section .bss
.align 4
stack:
    .skip 4096
stack_top:
