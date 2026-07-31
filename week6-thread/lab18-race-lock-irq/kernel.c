#include "thread.h"

#define MAX_THREADS 4
#define STACK_SIZE 4096
#define SSTATUS_SIE (1UL << 1)
#define UART_THR (*(volatile unsigned char *)0x10000000UL)
#define UART_LSR (*(volatile unsigned char *)0x10000005UL)

struct lock { struct thread *owner; struct condition released; };
static struct thread threads[MAX_THREADS];
static unsigned char stacks[MAX_THREADS][STACK_SIZE] __attribute__((aligned(16)));
static struct thread *ready_head, *ready_tail;
static u64 live_workers;
static volatile u64 shared_counter;
static struct lock counter_lock;
static volatile u64 event_count, event_checksum;
static volatile int isr_saw_broken_invariant;

static void putc(char c) { while (!(UART_LSR & 0x20)) {} UART_THR = (unsigned char)c; }
static void puts(const char *s) { while (*s) putc(*s++); }
static struct thread *current(void) {
    struct thread *t; __asm__ volatile("mv %0, tp" : "=r"(t)); return t;
}
static void push(struct thread *t) {
    t->ready_next = 0;
    if (ready_tail) ready_tail->ready_next = t; else ready_head = t;
    ready_tail = t;
}
static struct thread *pop(void) {
    struct thread *t = ready_head;
    if (!t) return 0;
    ready_head = t->ready_next;
    if (!ready_head) ready_tail = 0;
    return t;
}
static void schedule(int requeue) {
    struct thread *old = current(), *next;
    if (requeue) { old->state = THREAD_READY; push(old); }
    next = pop();
    if (!next) {
        if (old->state == THREAD_RUNNING) return;
        puts("LAB18 FAIL: no runnable thread\n");
        for (;;) __asm__ volatile("wfi");
    }
    next->state = THREAD_RUNNING;
    _swtch(next);
}
void thread_yield(void) { schedule(1); }
int thread_spawn(const char *name, thread_fn fn, void *arg) {
    int i;
    for (i = 1; i < MAX_THREADS; i++) if (threads[i].state == THREAD_UNUSED) break;
    if (i == MAX_THREADS) return -1;
    threads[i].id = (u64)i; threads[i].name = name;
    threads[i].stack_low = (u64)&stacks[i][0];
    threads[i].stack_high = (u64)&stacks[i][STACK_SIZE];
    threads[i].context.sp = threads[i].stack_high & ~15UL;
    threads[i].context.ra = (u64)thread_setup;
    threads[i].start_fn = fn; threads[i].start_arg = arg;
    threads[i].state = THREAD_READY; push(&threads[i]); live_workers++;
    return i;
}
void thread_exit(void) {
    current()->state = THREAD_EXITED; live_workers--; schedule(0);
    for (;;) {}
}
void condition_init(struct condition *c, const char *name) {
    c->name = name; c->wait_head = c->wait_tail = 0;
}
void condition_wait(struct condition *c) {
    struct thread *t = current(); t->state = THREAD_WAITING; t->wait_next = 0;
    if (c->wait_tail) c->wait_tail->wait_next = t; else c->wait_head = t;
    c->wait_tail = t; schedule(0);
}
void condition_broadcast(struct condition *c) {
    struct thread *t = c->wait_head;
    while (t) {
        struct thread *n = t->wait_next;
        t->wait_next = 0; t->state = THREAD_READY; push(t); t = n;
    }
    c->wait_head = c->wait_tail = 0;
}
static void lock_init(struct lock *l) {
    l->owner = 0; condition_init(&l->released, "lock_released");
}
static void lock_acquire(struct lock *l) {
    __asm__ volatile(".globl lock_acquire_observe\nlock_acquire_observe:");
#if LAB18_SOLUTION
    while (l->owner != 0) condition_wait(&l->released);
    l->owner = current();
#else
    (void)l;
    puts("[exercise] complete lock acquire/release after observing reference\n");
    for (;;) __asm__ volatile("wfi");
#endif
}
static void lock_release(struct lock *l) {
#if LAB18_SOLUTION
    if (l->owner != current()) {
        puts("LAB18 FAIL: non-owner release\n");
        for (;;) __asm__ volatile("wfi");
    }
    l->owner = 0; condition_broadcast(&l->released);
#else
    (void)l;
#endif
    __asm__ volatile(".globl lock_release_observe\nlock_release_observe:");
}
static void unsafe_worker(void *unused) {
    u64 v; (void)unused;
    v = shared_counter;
    __asm__ volatile(".globl counter_loaded\ncounter_loaded:");
    thread_yield();
    __asm__ volatile(".globl counter_before_store\ncounter_before_store:");
    shared_counter = v + 1;
}
static void safe_worker(void *unused) {
    u64 v; (void)unused;
    lock_acquire(&counter_lock);
    v = shared_counter;
    thread_yield();
    shared_counter = v + 1;
    lock_release(&counter_lock);
}
static void wait_workers(void) { while (live_workers) thread_yield(); }
static void recycle_workers(void) {
    int i; for (i = 1; i < MAX_THREADS; i++) threads[i].state = THREAD_UNUSED;
}
static u64 irq_save_disable(void) {
    u64 old;
    __asm__ volatile(".globl irq_save_disable_observe\nirq_save_disable_observe:");
#if LAB18_SOLUTION
    __asm__ volatile("csrrc %0, sstatus, %1" : "=r"(old) : "r"(SSTATUS_SIE) : "memory");
#else
    old = 0;
    puts("[exercise] complete irq save-disable-restore\n");
    for (;;) __asm__ volatile("wfi");
#endif
    return old;
}
static void irq_restore(u64 old) {
#if LAB18_SOLUTION
    if (old & SSTATUS_SIE) __asm__ volatile("csrs sstatus, %0" : : "r"(SSTATUS_SIE) : "memory");
    else __asm__ volatile("csrc sstatus, %0" : : "r"(SSTATUS_SIE) : "memory");
#else
    (void)old;
#endif
    __asm__ volatile(".globl irq_restore_observe\nirq_restore_observe:");
}
static void simulated_isr(void) {
    if (event_checksum != event_count * 3) isr_saw_broken_invariant = 1;
}
static void unprotected_event_update(void) {
    event_count++;
    simulated_isr();
    event_checksum = event_count * 3;
}
static void protected_event_update(void) {
    u64 old = irq_save_disable();
    event_count++;
    event_checksum = event_count * 3;
    irq_restore(old);
    simulated_isr();
}
void lab18_main(void) {
    struct thread *m = &threads[0];
    m->id = 0; m->name = "main"; m->state = THREAD_RUNNING;
    __asm__ volatile("mv tp, %0" : : "r"(m) : "memory");
    shared_counter = 0;
    thread_spawn("unsafe-A", unsafe_worker, 0);
    thread_spawn("unsafe-B", unsafe_worker, 0);
    wait_workers();
    if (shared_counter != 1) { puts("LAB18 FAIL: race not reproduced\n"); for (;;) {} }
    puts("[Part A] deterministic race observed\n");
    recycle_workers(); shared_counter = 0; lock_init(&counter_lock);
    thread_spawn("safe-A", safe_worker, 0);
    thread_spawn("safe-B", safe_worker, 0);
    wait_workers();
    if (shared_counter != 2) { puts("LAB18 FAIL: lock\n"); for (;;) {} }
    puts("[Part B] lock preserves counter\n");
    event_count = event_checksum = 0; isr_saw_broken_invariant = 0;
    unprotected_event_update();
    if (!isr_saw_broken_invariant) { puts("LAB18 FAIL: ISR baseline\n"); for (;;) {} }
    puts("[Part C] unprotected ISR observation reproduced\n");
    event_count = event_checksum = 0; isr_saw_broken_invariant = 0;
    protected_event_update();
    if (isr_saw_broken_invariant) { puts("LAB18 FAIL: IRQ critical section\n"); for (;;) {} }
    puts("[Part C] interrupt state restored\nLAB18 PASS\n");
    for (;;) __asm__ volatile("wfi");
}
