.equ SSTATUS_SPP, (1 << 8)

.section .text
.globl enter_user
.globl before_user_sret

# void enter_user(unsigned long entry, unsigned long user_stack_top)
enter_user:
    # Framework code inherited from Lab 14.
    csrw sepc, a0
    li t0, SSTATUS_SPP
    csrc sstatus, t0

    # On a U->S trap, hardware does not switch stacks. sscratch remembers the
    # kernel stack while sp is changed to the user stack.
    csrw sscratch, sp
    mv sp, a1

before_user_sret:
    sret
