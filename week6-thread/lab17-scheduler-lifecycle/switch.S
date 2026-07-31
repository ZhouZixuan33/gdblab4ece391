#include "thread.h"
.section .text
.globl _swtch
_swtch:
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
    mv tp, a0
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
    ret

.globl thread_setup
.globl thread_setup_entry
thread_setup:
thread_setup_entry:
    ld t0, THREAD_START_FN(tp)
    ld a0, THREAD_START_ARG(tp)
    jalr ra, t0, 0
.globl thread_function_returned
thread_function_returned:
    call thread_exit
1:  j 1b
