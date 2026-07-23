.equ SSTATUS_SPP, (1 << 8)

.section .text
.globl enter_user
.globl before_user_sret

# void enter_user(unsigned long entry, unsigned long user_stack_top)
enter_user:
#if LAB14_SOLUTION
    # Reference solution for LAB14 TODO 4.
    csrw sepc, a0
    li t0, SSTATUS_SPP
    csrc sstatus, t0

    # On a U->S trap, hardware does not switch stacks. sscratch remembers the
    # kernel stack while sp is changed to the user stack.
    csrw sscratch, sp
    mv sp, a1
#else
    # LAB14 TODO 4 (exercise):
    #   1. write the ELF entry in a0 to sepc;
    #   2. clear sstatus.SPP;
    #   3. save the kernel sp in sscratch;
    #   4. install the user stack from a1.
    csrw sepc, zero
    csrw sscratch, sp
    mv sp, a1
#endif

before_user_sret:
    sret
