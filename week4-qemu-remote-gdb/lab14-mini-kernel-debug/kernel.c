#include "elf_loader.h"

#define UART0_BASE 0x10000000UL
#define UART_THR (*(volatile unsigned char *)(UART0_BASE + 0))
#define UART_LSR (*(volatile unsigned char *)(UART0_BASE + 5))
#define UART_LSR_THRE 0x20
#define USER_STACK_TOP 0x80410000UL

extern const unsigned char embedded_user_elf_start[];
extern const unsigned char embedded_user_elf_end[];
extern void supervisor_trap_entry(void);
extern void enter_user(u64 entry, u64 user_stack_top);

volatile struct loaded_user loaded_user;
volatile u64 lab14_exit_code;

static void uart_putchar(char value) {
    while ((UART_LSR & UART_LSR_THRE) == 0)
        ;
    UART_THR = (unsigned char)value;
}

void uart_puts(const char *text) {
    while (*text != '\0')
        uart_putchar(*text++);
}

static void write_stvec(u64 value) {
    __asm__ volatile("csrw stvec, %0" : : "r"(value));
}

static u64 read_scause(void) {
    u64 value;
    __asm__ volatile("csrr %0, scause" : "=r"(value));
    return value;
}

static u64 read_sepc(void) {
    u64 value;
    __asm__ volatile("csrr %0, sepc" : "=r"(value));
    return value;
}

void elf_load_done(void) {
    __asm__ volatile("" : : : "memory");
}

__attribute__((noreturn))
void kernel_exit_success(void) {
    uart_puts("[U] probe returned\n");
    uart_puts("[S] user exit: 0\n");
    uart_puts("LAB14 PASS\n");
    for (;;)
        __asm__ volatile("wfi");
}

__attribute__((noreturn))
void kernel_exit_failure(void) {
    uart_puts("[S] user exit reported failure\n");
    uart_puts("LAB14 FAIL\n");
    for (;;)
        __asm__ volatile("wfi");
}

__attribute__((noreturn))
void kernel_exit_from_user(u64 exit_code) {
    lab14_exit_code = exit_code;
    if (exit_code == 0)
        kernel_exit_success();
    kernel_exit_failure();
}

__attribute__((noreturn))
void kernel_unexpected_trap(void) {
    volatile u64 cause = read_scause();
    volatile u64 pc = read_sepc();
    (void)cause;
    (void)pc;
    uart_puts("[S] unexpected trap; inspect scause and sepc in GDB\n");
    for (;;)
        __asm__ volatile("wfi");
}

__attribute__((noreturn))
void supervisor_entry(void) {
    u64 image_size = (u64)(embedded_user_elf_end - embedded_user_elf_start);
    int status;

    __asm__ volatile("csrw satp, zero\nsfence.vma" : : : "memory");

    uart_puts("[S] kernel\n");

#if LAB14_SOLUTION
    /* Reference solution for LAB14 TODO 1. */
    write_stvec((u64)supervisor_trap_entry);
#else
    /* LAB14 TODO 1 (exercise): install supervisor_trap_entry in stvec. */
    write_stvec(0);
#endif

    status = load_user_elf(embedded_user_elf_start, image_size,
                           (struct loaded_user *)&loaded_user);
    if (status != 0) {
        uart_puts("[S] user ELF load failed\n");
        kernel_exit_failure();
    }

    uart_puts("[S] user ELF loaded\n");
    elf_load_done();
    enter_user(loaded_user.entry, USER_STACK_TOP);

    kernel_exit_failure();
}
