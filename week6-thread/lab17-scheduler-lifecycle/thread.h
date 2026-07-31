#ifndef LAB17_THREAD_H
#define LAB17_THREAD_H

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
#define THREAD_START_FN 168
#define THREAD_START_ARG 176

#ifndef __ASSEMBLER__
typedef unsigned long u64;
typedef void (*thread_fn)(void *);

enum thread_state {
    THREAD_UNUSED,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_WAITING,
    THREAD_EXITED
};

struct thread_context { u64 ra, sp, s[12]; };

struct thread {
    struct thread_context context;
    u64 id;
    enum thread_state state;
    const char *name;
    u64 stack_low, stack_high;
    struct thread *ready_next;
    struct thread *wait_next;
    thread_fn start_fn;
    void *start_arg;
};

struct condition {
    const char *name;
    struct thread *wait_head;
    struct thread *wait_tail;
};

_Static_assert(__builtin_offsetof(struct thread, start_fn) == THREAD_START_FN, "start_fn");
_Static_assert(__builtin_offsetof(struct thread, start_arg) == THREAD_START_ARG, "start_arg");

void _swtch(struct thread *next);
void thread_setup(void);
void thread_exit(void) __attribute__((noreturn));
void thread_yield(void);
int thread_spawn(const char *name, thread_fn fn, void *arg);
void condition_init(struct condition *cond, const char *name);
void condition_wait(struct condition *cond);
void condition_broadcast(struct condition *cond);
#endif
#endif
