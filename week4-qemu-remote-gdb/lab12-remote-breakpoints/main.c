/*
 * Lab 12 C-side bare-metal entry.
 *
 * This is freestanding C: there is no guest OS process, libc, printf, or
 * default C runtime. start.S establishes sp and calls kernel_entry explicitly.
 * Nothing in this file changes privilege mode, so these functions continue to
 * execute in M-mode.
 */

/*
 * UART (Universal Asynchronous Receiver/Transmitter) converts bytes between
 * software and a serial character stream. With QEMU -nographic, transmitted
 * characters appear in the host terminal.
 *
 * MMIO (memory-mapped I/O) gives device registers addresses in the CPU address
 * space. Loads and stores at UART0_BASE access the emulated UART, not RAM.
 * This lab writes characters to the transmit register at offset 0 and reads
 * status from the line-status register at offset 5.
 */
#define UART0_BASE 0x10000000UL
/* Register 5 is the line-status register; bit 5 means TX can accept a byte. */
#define UART_LINE_STATUS 5
#define UART_TX_READY 0x20

/*
 * 这些具名全局变量记录程序执行进度，GDB 可以通过 ELF 符号查看它们。
 * 仅仅因为要用 GDB 观察变量，并不需要把变量声明为 volatile。本实验带有
 * 调试信息，并且没有开启编译优化，因此这些变量会保留清晰的内存位置。
 */
unsigned int debug_state;
unsigned int scheduler_state;
int main_return;

static void uart_putc(char ch) {
    /*
     * volatile 的中文意思是“易变的”。UART 是 MMIO 设备：状态寄存器的
     * 值可能被设备自行改变，写发送寄存器本身也会触发设备行为。因此每次
     * 读取和写入都必须真正到达设备，编译器不能缓存、合并或删除这些访问。
     *
     */
    volatile unsigned char *uart =
        (volatile unsigned char *)UART0_BASE;

    /* Poll the transmitter-ready bit; this lab does not use UART interrupts. */
    while ((uart[UART_LINE_STATUS] & UART_TX_READY) == 0) {
    }

    /* Register 0 is the transmit holding register when writing. */
    uart[0] = (unsigned char)ch;
}

/* Minimal string output for a target that has no libc puts or printf. */
static void uart_puts(const char *text) {
    while (*text != '\0') {
        uart_putc(*text);
        text++;
    }
}

/*
 * This is a named startup boundary for GDB, not a complete UART driver setup.
 * QEMU presents the UART ready for this minimal polling example. The Makefile
 * uses -O0, so this function remains a clear symbolic breakpoint in the lab.
 */
void init_console(void) {
    uart_puts("Lab 12 init_console reached\n");
}

/* Write a known value so GDB can prove this checkpoint executed. */
void debug_checkpoint(void) {
    debug_state = 0x39141201U;
    uart_puts("Lab 12 debug_checkpoint reached\n");
}

/* A second named checkpoint demonstrates continuing between breakpoints. */
void scheduler_checkpoint(void) {
    scheduler_state = 0x39141202U;
    uart_puts("Lab 12 scheduler_checkpoint reached\n");
}

/*
 * In a hosted program, the C runtime calls main. In this bare-metal target,
 * kernel_entry calls main explicitly after start.S has established the stack.
 */
int main(void) {
    uart_puts("Lab 12 main reached\n");
    debug_checkpoint();
    scheduler_checkpoint();
    uart_puts("Lab 12 done; CPU will wait in halt_loop\n");
    return 0;
}

/*
 * First C function reached from start.S. This is an assembly-to-C software
 * boundary, not a privilege transition. It calls the console boundary and
 * main, then records main's ABI return value so GDB can inspect it.
 */
int kernel_entry(void) {
    int result;

    init_console();
    result = main();
    main_return = result;
    return result;
}
