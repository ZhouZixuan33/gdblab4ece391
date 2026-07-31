#include "thread.h"

.section .text
.balign 4
.globl _swtch
.globl swtch_save_done
.globl swtch_tp_changed
.globl swtch_restore_done

_swtch:
#if LAB16_SOLUTION
    sd ra, CTX_RA(tp)
    sd sp, CTX_SP(tp)
    sd s0, CTX_S0(tp)
    sd s1, CTX_S1(tp)
    sd s2, CTX_S2(tp)
    sd s3, CTX_S3(tp)
    sd s4, CTX_S4(tp)
    sd s5, CTX_S5(tp)
    sd s6, CTX_S6(tp)
    sd s7, CTX_S7(tp)
    sd s8, CTX_S8(tp)
    sd s9, CTX_S9(tp)
    sd s10, CTX_S10(tp)
    sd s11, CTX_S11(tp)
swtch_save_done:
    mv tp, a0
swtch_tp_changed:
    ld ra, CTX_RA(tp)
    ld sp, CTX_SP(tp)
    ld s0, CTX_S0(tp)
    ld s1, CTX_S1(tp)
    ld s2, CTX_S2(tp)
    ld s3, CTX_S3(tp)
    ld s4, CTX_S4(tp)
    ld s5, CTX_S5(tp)
    ld s6, CTX_S6(tp)
    ld s7, CTX_S7(tp)
    ld s8, CTX_S8(tp)
    ld s9, CTX_S9(tp)
    ld s10, CTX_S10(tp)
    ld s11, CTX_S11(tp)
swtch_restore_done:
    ret
#else
    j lab16_todo_checkpoint
swtch_save_done:
swtch_tp_changed:
swtch_restore_done:
    ret
#endif

.globl thread_a_flow
.globl thread_a_before_switch
.globl thread_a_after_resume
thread_a_flow:
    addi sp, sp, -16
    sd ra, 8(sp)
    li s1, 0xa16
    li s2, 50
thread_a_before_switch:
    la a0, thread_b
    call _swtch
thread_a_after_resume:
    li t0, 0xa16
    bne s1, t0, context_check_failed
    addi s2, s2, -1
    beqz s2, 1f
    la a0, thread_b
    call _swtch
    j thread_a_after_resume
1:
    ld ra, 8(sp)
    addi sp, sp, 16
    ret

.globl thread_b_entry
.globl thread_b_after_switch
thread_b_entry:
    li s1, 0xb16
    li s2, 50
    la a0, thread_a
    call _swtch
thread_b_after_switch:
    li t0, 0xb16
    bne s1, t0, context_check_failed
    addi s2, s2, -1
    la a0, thread_a
    call _swtch
    j thread_b_after_switch

context_check_failed:
    call lab16_fail
1:  j 1b
