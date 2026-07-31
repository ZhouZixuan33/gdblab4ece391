.equ MSTATUS_MPP_MASK, (3 << 11)
.equ MSTATUS_MPP_S, (1 << 11)
.section .text.boot
.globl _start
_start:
    la sp, boot_stack_top
    li t0, -1
    csrw pmpaddr0, t0
    li t0, 0x0f
    csrw pmpcfg0, t0
    li t0, MSTATUS_MPP_MASK
    csrc mstatus, t0
    li t0, MSTATUS_MPP_S
    csrs mstatus, t0
    la t0, lab18_main
    csrw mepc, t0
    mret
.section .bss
.balign 16
boot_stack:
    .skip 8192
boot_stack_top:
