#ifndef LAB15_TRAP_FRAME_H
#define LAB15_TRAP_FRAME_H

#define TF_RA       0
#define TF_GP       8
#define TF_TP       16
#define TF_T0       24
#define TF_T1       32
#define TF_T2       40
#define TF_S0       48
#define TF_S1       56
#define TF_A0       64
#define TF_A1       72
#define TF_A2       80
#define TF_A3       88
#define TF_A4       96
#define TF_A5       104
#define TF_A6       112
#define TF_A7       120
#define TF_S2       128
#define TF_S3       136
#define TF_S4       144
#define TF_S5       152
#define TF_S6       160
#define TF_S7       168
#define TF_S8       176
#define TF_S9       184
#define TF_S10      192
#define TF_S11      200
#define TF_T3       208
#define TF_T4       216
#define TF_T5       224
#define TF_T6       232
#define TF_SP       240
#define TF_SEPC     248
#define TF_SSTATUS  256
#define TF_SCAUSE   264
#define TF_SIZE     272

#ifndef __ASSEMBLER__

typedef unsigned long u64;

struct trap_frame {
    u64 ra;
    u64 gp;
    u64 tp;
    u64 t0;
    u64 t1;
    u64 t2;
    u64 s0;
    u64 s1;
    u64 a0;
    u64 a1;
    u64 a2;
    u64 a3;
    u64 a4;
    u64 a5;
    u64 a6;
    u64 a7;
    u64 s2;
    u64 s3;
    u64 s4;
    u64 s5;
    u64 s6;
    u64 s7;
    u64 s8;
    u64 s9;
    u64 s10;
    u64 s11;
    u64 t3;
    u64 t4;
    u64 t5;
    u64 t6;
    u64 sp;
    u64 sepc;
    u64 sstatus;
    u64 scause;
};

_Static_assert(sizeof(struct trap_frame) == TF_SIZE,
               "trap_frame layout must match trap.S");
#define TF_OFFSET_CHECK(field, offset) \
    _Static_assert(__builtin_offsetof(struct trap_frame, field) == (offset), \
                   "trap_frame offset mismatch: " #field)

TF_OFFSET_CHECK(ra, TF_RA);
TF_OFFSET_CHECK(s1, TF_S1);
TF_OFFSET_CHECK(a0, TF_A0);
TF_OFFSET_CHECK(s11, TF_S11);
TF_OFFSET_CHECK(t6, TF_T6);
TF_OFFSET_CHECK(sp, TF_SP);
TF_OFFSET_CHECK(sepc, TF_SEPC);
TF_OFFSET_CHECK(sstatus, TF_SSTATUS);
TF_OFFSET_CHECK(scause, TF_SCAUSE);

#undef TF_OFFSET_CHECK

#endif
#endif
