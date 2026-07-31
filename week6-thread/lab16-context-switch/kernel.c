#include "thread.h"

#define UART_THR (*(volatile unsigned char *)0x10000000UL)
#define UART_LSR (*(volatile unsigned char *)0x10000005UL)
#define UART_THRE 0x20

struct thread thread_a;
struct thread thread_b;
static unsigned char stack_b[4096] __attribute__((aligned(16)));

static void puts(const char *s) {
    while (*s) {
        while ((UART_LSR & UART_THRE) == 0) {}
        UART_THR = (unsigned char)*s++;
    }
}

void lab16_todo_checkpoint(void) {
    puts("[exercise] complete the three _swtch stages; inspect the solution reference first\n");
    for (;;) __asm__ volatile("wfi");
}

void lab16_fail(void) {
    puts("LAB16 FAIL: saved-register sentinel changed\n");
    for (;;) __asm__ volatile("wfi");
}

void lab16_main(void) {
    u64 btop = (u64)(stack_b + sizeof(stack_b));
    thread_a.id = 0;
    thread_a.name = "A";
    thread_b.id = 1;
    thread_b.name = "B";
    thread_b.stack_low = (u64)stack_b;
    thread_b.stack_high = btop;
    thread_b.context.sp = btop & ~15UL;
    thread_b.context.ra = (u64)thread_b_entry;
    __asm__ volatile("mv tp, %0" : : "r"(&thread_a) : "memory");
    puts("[A] before switch\n");
    thread_a_flow();
    puts("[A] resumed with its own stack and s-registers\n");
    puts("LAB16 PASS\n");
    for (;;) __asm__ volatile("wfi");
}
