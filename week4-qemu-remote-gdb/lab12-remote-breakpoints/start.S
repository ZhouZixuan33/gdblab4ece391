.equ UART0, 0x10000000

.section .text
.globl _start
.globl kernel_entry
.globl init_console
.globl debug_checkpoint
.globl scheduler_checkpoint
.globl halt_loop

_start:
    la sp, stack_top
    call kernel_entry

halt_loop:
    wfi
    j halt_loop

kernel_entry:
    call init_console
    la a0, msg_entry
    call uart_puts
    call debug_checkpoint
    call scheduler_checkpoint
    la a0, msg_done
    call uart_puts
    ret

init_console:
    la a0, msg_console
    call uart_puts
    ret

debug_checkpoint:
    la t0, debug_state
    li t1, 0x39141201
    sw t1, 0(t0)
    la a0, msg_debug
    call uart_puts
    ret

scheduler_checkpoint:
    la t0, scheduler_state
    li t1, 0x39141202
    sw t1, 0(t0)
    la a0, msg_scheduler
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
msg_console:
    .asciz "Lab 12 init_console reached\n"
msg_entry:
    .asciz "Lab 12 kernel_entry reached\n"
msg_debug:
    .asciz "Lab 12 debug_checkpoint reached\n"
msg_scheduler:
    .asciz "Lab 12 scheduler_checkpoint reached\n"
msg_done:
    .asciz "Lab 12 done; CPU will wait in halt_loop\n"

.section .data
debug_state:
    .word 0
scheduler_state:
    .word 0

.section .bss
.align 4
stack:
    .skip 4096
stack_top:
