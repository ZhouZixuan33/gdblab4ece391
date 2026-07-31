.equ MSTATUS_MPP_MASK, (3 << 11)
.equ MSTATUS_MPP_S,    (1 << 11)
.equ MEDELEG_ECALL_U,  (1 << 8)
.equ MIDELEG_SEIP,     (1 << 9)

.section .text.boot
.globl _start
.globl before_mret
.globl machine_trap_failure

_start:
    la sp, kernel_stack_top

    # A trap vector is installed before changing privilege state so early
    # configuration mistakes stop at a stable GDB checkpoint.
    la t0, machine_trap_failure
    csrw mtvec, t0

    # Permit S/U-mode accesses across the QEMU RAM range. The lab intentionally
    # has no memory isolation; a later virtual-memory lab adds that boundary.
    li t0, -1
    csrw pmpaddr0, t0
    li t0, 0x0f
    csrw pmpcfg0, t0

    # Delegate "environment call from U-mode" (exception cause 8) to S-mode.
    li t0, MEDELEG_ECALL_U
    csrs medeleg, t0

    # QEMU's PLIC context 1 delivers external interrupts to S-mode. Delegate
    # supervisor external interrupts so they do not remain in M-mode.
    li t0, MIDELEG_SEIP
    csrs mideleg, t0

    la a0, msg_machine_boot
    call uart_puts

    # mret will enter S-mode.
    li t0, MSTATUS_MPP_MASK
    csrc mstatus, t0
    li t0, MSTATUS_MPP_S
    csrs mstatus, t0

    # mret will continue at the S-mode kernel entry.
    la t0, supervisor_entry
    csrw mepc, t0

before_mret:
    mret

machine_trap_failure:
    la a0, msg_machine_trap
    call uart_puts
1:
    wfi
    j 1b

.section .rodata
msg_machine_boot:
    .asciz "[M] boot\n"
msg_machine_trap:
    .asciz "[M] unexpected trap\n"

.section .bss
.balign 16
.globl kernel_stack
.globl kernel_stack_top
kernel_stack:
    .skip 8192
kernel_stack_top:
