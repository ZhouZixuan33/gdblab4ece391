#ifndef LAB16_THREAD_H
#define LAB16_THREAD_H

#define CTX_RA 0
#define CTX_SP 8
#define CTX_S0 16
#define CTX_S1 24
#define CTX_S2 32
#define CTX_S3 40
#define CTX_S4 48
#define CTX_S5 56
#define CTX_S6 64
#define CTX_S7 72
#define CTX_S8 80
#define CTX_S9 88
#define CTX_S10 96
#define CTX_S11 104
#define CTX_SIZE 112

#ifndef __ASSEMBLER__
typedef unsigned long u64;

struct thread_context {
    u64 ra;
    u64 sp;
    u64 s[12];
};

struct thread {
    struct thread_context context;
    u64 id;
    const char *name;
    u64 stack_low;
    u64 stack_high;
};

_Static_assert(sizeof(struct thread_context) == CTX_SIZE, "context size");
_Static_assert(__builtin_offsetof(struct thread_context, ra) == CTX_RA, "ra offset");
_Static_assert(__builtin_offsetof(struct thread_context, sp) == CTX_SP, "sp offset");
_Static_assert(__builtin_offsetof(struct thread_context, s[11]) == CTX_S11, "s11 offset");

void _swtch(struct thread *next);
void thread_a_flow(void);
void thread_b_entry(void);
extern struct thread thread_a;
extern struct thread thread_b;
#endif
#endif
