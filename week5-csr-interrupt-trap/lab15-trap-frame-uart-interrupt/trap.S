#include "trap_frame.h"

.section .text
.balign 4
.globl supervisor_trap_entry
.globl trap_frame_saved
.globl before_trap_sret

supervisor_trap_entry:
    # Hardware does not switch stacks on a trap. Exchange the user sp with the
    # kernel sp that enter_user stored in sscratch.
    csrrw sp, sscratch, sp
    addi sp, sp, -TF_SIZE

    sd ra, TF_RA(sp)
    sd gp, TF_GP(sp)
    sd tp, TF_TP(sp)
    sd t0, TF_T0(sp)
    sd t1, TF_T1(sp)
    sd t2, TF_T2(sp)
    sd s0, TF_S0(sp)
#if LAB15_SOLUTION
    # Reference solution for LAB15 TODO 1.
    sd s1, TF_S1(sp)
#else
    # LAB15 TODO 1 (exercise): save s1 in its own trap-frame slot.
    sd zero, TF_S1(sp)
#endif
    sd a0, TF_A0(sp)
    sd a1, TF_A1(sp)
    sd a2, TF_A2(sp)
    sd a3, TF_A3(sp)
    sd a4, TF_A4(sp)
    sd a5, TF_A5(sp)
    sd a6, TF_A6(sp)
    sd a7, TF_A7(sp)
    sd s2, TF_S2(sp)
    sd s3, TF_S3(sp)
    sd s4, TF_S4(sp)
    sd s5, TF_S5(sp)
    sd s6, TF_S6(sp)
    sd s7, TF_S7(sp)
    sd s8, TF_S8(sp)
    sd s9, TF_S9(sp)
    sd s10, TF_S10(sp)
    sd s11, TF_S11(sp)
    sd t3, TF_T3(sp)
    sd t4, TF_T4(sp)
    sd t5, TF_T5(sp)
    sd t6, TF_T6(sp)

    # sscratch currently holds the user sp that was exchanged on entry.
    csrr t0, sscratch
    sd t0, TF_SP(sp)
    csrr t0, sepc
    sd t0, TF_SEPC(sp)
    csrr t0, sstatus
    sd t0, TF_SSTATUS(sp)
    csrr t0, scause
    sd t0, TF_SCAUSE(sp)

trap_frame_saved:
#if LAB15_SOLUTION
    # Reference solution for LAB15 TODO 2.
    mv a0, sp
#else
    # LAB15 TODO 2 (exercise): pass the trap-frame base address in a0.
    addi a0, sp, TF_SIZE
#endif
    call supervisor_trap_dispatch

    # The C dispatcher may update sepc/sstatus in the frame.
    ld t0, TF_SEPC(sp)
    csrw sepc, t0
    ld t0, TF_SSTATUS(sp)
    csrw sstatus, t0

    ld ra, TF_RA(sp)
    ld gp, TF_GP(sp)
    ld tp, TF_TP(sp)
    ld t1, TF_T1(sp)
    ld t2, TF_T2(sp)
    ld s0, TF_S0(sp)
#if LAB15_SOLUTION
    # Reference solution for LAB15 TODO 6.
    ld s1, TF_S1(sp)
#else
    # LAB15 TODO 6 (exercise): restore s1 from TF_S1, not TF_S0.
    ld s1, TF_S0(sp)
#endif
    ld a0, TF_A0(sp)
    ld a1, TF_A1(sp)
    ld a2, TF_A2(sp)
    ld a3, TF_A3(sp)
    ld a4, TF_A4(sp)
    ld a5, TF_A5(sp)
    ld a6, TF_A6(sp)
    ld a7, TF_A7(sp)
    ld s2, TF_S2(sp)
    ld s3, TF_S3(sp)
    ld s4, TF_S4(sp)
    ld s5, TF_S5(sp)
    ld s6, TF_S6(sp)
    ld s7, TF_S7(sp)
    ld s8, TF_S8(sp)
    ld s9, TF_S9(sp)
    ld s10, TF_S10(sp)
    ld s11, TF_S11(sp)
    ld t3, TF_T3(sp)
    ld t4, TF_T4(sp)
    ld t5, TF_T5(sp)
    ld t6, TF_T6(sp)
    ld t0, TF_T0(sp)

before_trap_sret:
    addi sp, sp, TF_SIZE
    csrrw sp, sscratch, sp
    sret

