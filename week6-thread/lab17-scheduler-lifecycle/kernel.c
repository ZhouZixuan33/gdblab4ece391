#include "thread.h"

#define MAX_THREADS 6
#define STACK_SIZE 4096
#define UART_THR (*(volatile unsigned char *)0x10000000UL)
#define UART_LSR (*(volatile unsigned char *)0x10000005UL)

static struct thread threads[MAX_THREADS];
static unsigned char stacks[MAX_THREADS][STACK_SIZE] __attribute__((aligned(16), used));
static struct thread *ready_head, *ready_tail;
static u64 live_workers, switch_count;
static struct condition data_ready;
static int event_ready;

static void putc(char c) {
    while ((UART_LSR & 0x20) == 0) {}
    UART_THR = (unsigned char)c;
}
static void puts(const char *s) { while (*s) putc(*s++); }
static struct thread *current(void) {
    struct thread *value;
    __asm__ volatile("mv %0, tp" : "=r"(value));
    return value;
}
static void ready_push(struct thread *t) {
    t->ready_next = 0;
    if (ready_tail) ready_tail->ready_next = t; else ready_head = t;
    ready_tail = t;
}
static struct thread *ready_pop(void) {
    struct thread *t = ready_head;
    if (!t) return 0;
    ready_head = t->ready_next;
    if (!ready_head) ready_tail = 0;
    t->ready_next = 0;
    return t;
}
void lab17_todo_checkpoint(void) {
    puts("[exercise] complete spawn, RR yield, and exit TODOs\n");
    for (;;) __asm__ volatile("wfi");
}
static void schedule(int requeue_current) {
    struct thread *old = current();
    struct thread *next;
    if (requeue_current) {
        old->state = THREAD_READY;
        ready_push(old);
    }
    __asm__ volatile(".globl scheduler_before_switch\nscheduler_before_switch:");
    next = ready_pop();
    if (!next) {
        if (old->state == THREAD_RUNNING) return;
        puts("LAB17 FAIL: no runnable thread\n");
        for (;;) __asm__ volatile("wfi");
    }
    next->state = THREAD_RUNNING;
    switch_count++;
    __asm__ volatile(".globl scheduler_after_select\nscheduler_after_select:");
    _swtch(next);
}
void thread_yield(void) {
#if LAB17_SOLUTION
    schedule(1);
#else
    lab17_todo_checkpoint();
#endif
}
int thread_spawn(const char *name, thread_fn fn, void *arg) {
    u64 i;
    struct thread *t = 0;
    for (i = 1; i < MAX_THREADS; i++)
        if (threads[i].state == THREAD_UNUSED) { t = &threads[i]; break; }
    if (!t) return -1;
    (void)name; (void)fn; (void)arg;
#if LAB17_SOLUTION
    t->id = i;
    t->name = name;
    t->stack_low = (u64)&stacks[i][0];
    t->stack_high = (u64)&stacks[i][STACK_SIZE];
    t->context.sp = t->stack_high & ~15UL;
    t->context.ra = (u64)thread_setup;
    t->start_fn = fn;
    t->start_arg = arg;
    t->state = THREAD_READY;
    ready_push(t);
#else
    lab17_todo_checkpoint();
#endif
    __asm__ volatile(".globl thread_created\nthread_created:");
    live_workers++;
    return (int)i;
}
void thread_exit(void) {
    __asm__ volatile(".globl thread_marked_exited\nthread_marked_exited:");
#if LAB17_SOLUTION
    current()->state = THREAD_EXITED;
    live_workers--;
    schedule(0);
#else
    lab17_todo_checkpoint();
#endif
    for (;;) {}
}
void condition_init(struct condition *c, const char *name) {
    c->name = name; c->wait_head = c->wait_tail = 0;
}
void condition_wait(struct condition *c) {
    struct thread *t = current();
    __asm__ volatile(".globl condition_wait_entered\ncondition_wait_entered:");
    t->state = THREAD_WAITING;
    t->wait_next = 0;
    if (c->wait_tail) c->wait_tail->wait_next = t; else c->wait_head = t;
    c->wait_tail = t;
    __asm__ volatile(".globl condition_thread_sleeping\ncondition_thread_sleeping:");
    schedule(0);
    __asm__ volatile(".globl condition_thread_resumed\ncondition_thread_resumed:");
}
void condition_broadcast(struct condition *c) {
    struct thread *t = c->wait_head;
    while (t) {
        struct thread *next = t->wait_next;
        t->wait_next = 0;
        t->state = THREAD_READY;
        ready_push(t);
        t = next;
    }
    c->wait_head = c->wait_tail = 0;
    __asm__ volatile(".globl condition_broadcast_done\ncondition_broadcast_done:");
}
static void worker(void *arg) {
    u64 rounds = (u64)arg;
    while (rounds--) thread_yield();
}
static void waiter(void *unused) {
    (void)unused;
    while (!event_ready) condition_wait(&data_ready);
    puts("[waiter] resumed after broadcast\n");
}
void lab17_main(void) {
    struct thread *main_t = &threads[0];
    main_t->id = 0; main_t->name = "main"; main_t->state = THREAD_RUNNING;
    __asm__ volatile("mv tp, %0" : : "r"(main_t) : "memory");
    condition_init(&data_ready, "data_ready");
    thread_spawn("A", worker, (void *)3);
    thread_spawn("B", worker, (void *)2);
    thread_spawn("C", worker, (void *)1);
    thread_spawn("waiter", waiter, 0);
    while (live_workers) {
        if (!event_ready && threads[4].state == THREAD_WAITING) {
            event_ready = 1;
            condition_broadcast(&data_ready);
        }
        thread_yield();
    }
    if (switch_count < 10) puts("LAB17 FAIL: too few switches\n");
    else puts("LAB17 PASS\n");
    for (;;) __asm__ volatile("wfi");
}
