#define UART0_BASE 0x10000000UL
#define UART_LSR_OFFSET 5
#define UART_LSR_TX_READY 0x20

long asm_add_three(long first, long second, long third);
int check_preserves_s1(void);

volatile long g_add_result;
volatile int g_check_result;
volatile int g_done;
volatile long g_main_return;

static void uart_putc(char ch) {
    volatile unsigned char *uart = (volatile unsigned char *)UART0_BASE;

    while ((uart[UART_LSR_OFFSET] & UART_LSR_TX_READY) == 0) {
    }

    uart[0] = (unsigned char)ch;
}

void uart_puts(const char *text) {
    while (*text != '\0') {
        uart_putc(*text);
        text++;
    }
}

__attribute__((noinline)) void runtime_before_add(void) {
    __asm__ volatile("");
}

__attribute__((noinline)) void runtime_after_add(void) {
    __asm__ volatile("");
}

__attribute__((noinline)) void runtime_before_preserve_check(void) {
    __asm__ volatile("");
}

__attribute__((noinline)) void runtime_after_preserve_check(void) {
    __asm__ volatile("");
}

int kernel_entry(void) {
    uart_puts("Lab 11 runtime target reached\n");

    runtime_before_add();
    g_add_result = asm_add_three(100, 20, 1);
    runtime_after_add();

    runtime_before_preserve_check();
    g_check_result = check_preserves_s1();
    runtime_after_preserve_check();

    if (g_add_result == 121) {
        uart_puts("asm_add_three returned 121\n");
    } else {
        uart_puts("asm_add_three returned an unexpected value\n");
    }

    if (g_check_result != 0) {
        uart_puts("check_preserves_s1 detected clobbered s1\n");
    } else {
        uart_puts("check_preserves_s1 did not detect the bug\n");
    }

    uart_puts("Lab 11 done; CPU will wait in halt_loop\n");
    g_done = 1;

    return g_check_result;
}
