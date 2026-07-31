#include "elf_loader.h"
#include "trap_frame.h"
#include "uart.h"

#define USER_STACK_TOP 0x80410000UL

extern const unsigned char embedded_user_elf_start[];
extern const unsigned char embedded_user_elf_end[];
extern void supervisor_trap_entry(void);
extern void enter_user(u64 entry, u64 user_stack_top);

volatile struct loaded_user loaded_user;
volatile u64 lab15_exit_code;

static void write_stvec(u64 value) {
    __asm__ volatile("csrw stvec, %0" : : "r"(value));
}

void elf_load_done(void) {
    __asm__ volatile("" : : : "memory");
}

__attribute__((noreturn))
void kernel_exit_success(void) {
    uart_puts("[U] register sentinels preserved\n");
    uart_puts("[S] user exit: 0\n");
    uart_puts("LAB15 PASS\n");
    for (;;)
        __asm__ volatile("wfi");
}

__attribute__((noreturn))
void kernel_exit_failure(void) {
    uart_puts("[S] user reported register corruption\n");
    uart_puts("LAB15 FAIL\n");
    for (;;)
        __asm__ volatile("wfi");
}

__attribute__((noreturn))
void kernel_exit_from_user(u64 exit_code) {
    lab15_exit_code = exit_code;
    if (exit_code == 0)
        kernel_exit_success();
    kernel_exit_failure();
}

__attribute__((noreturn))
void kernel_unexpected_trap(struct trap_frame *frame) {
    volatile u64 cause = frame == (struct trap_frame *)0 ? ~0UL : frame->scause;
    volatile u64 pc = frame == (struct trap_frame *)0 ? 0 : frame->sepc;
    (void)cause;
    (void)pc;
    uart_puts("[S] unexpected trap; inspect trap frame in GDB\n");
    for (;;)
        __asm__ volatile("wfi");
}

__attribute__((noreturn))
void supervisor_entry(void) {
    u64 image_size = (u64)(embedded_user_elf_end - embedded_user_elf_start);
    int status;

    __asm__ volatile("csrw satp, zero\nsfence.vma" : : : "memory");
    uart_puts("[S] kernel\n");

    /* Framework code: install the trap entry before interrupts can be enabled. */
    write_stvec((u64)supervisor_trap_entry);

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

