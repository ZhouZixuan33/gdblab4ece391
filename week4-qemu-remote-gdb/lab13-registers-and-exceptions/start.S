/*
 * Lab 13 的全部裸机代码。
 *
 * 这个文件同时负责四件事：
 *   1. 从 _start 建立栈；
 *   2. 根据 SCENARIO_ID 选择一个实验场景；
 *   3. 直接通过 QEMU virt 的 UART 输出字符串；
 *   4. 故意制造坏地址、意外跳转和非法指令。
 *
 * 文件扩展名使用大写 .S，因此 GCC 会先运行 C preprocessor，再把结果
 * 交给 assembler。下面的 #if / #elif 就是在预处理阶段选择代码路径。
 */

/* QEMU virt 上 NS16550A-compatible UART 的 MMIO base address。 */
.equ UART0, 0x10000000

/*
 * Makefile 通常通过 -DSCENARIO_ID=... 定义该值。
 * 如果直接编译而没有定义，则默认选择 bad-pointer（编号 1）。
 */
#ifndef SCENARIO_ID
#define SCENARIO_ID 1
#endif

/* .text 保存 CPU 会执行的代码。 */
.section .text

/*
 * .globl 将 symbol 导出，使 linker 能用 _start 作为 ELF entry，
 * 也使 GDB 可以使用这些名字设置 breakpoint。
 */
.globl _start
.globl kernel_entry
.globl scenario_good
.globl scenario_bad_pointer
.globl bad_pointer_fault_site
.globl scenario_bad_jump
.globl bad_jump_source
.globl unexpected_landing
.globl scenario_illegal_instruction
.globl illegal_instruction_site
.globl halt_loop

/*
 * 裸机软件入口。
 *
 * 此时没有 OS 或 C runtime 帮我们准备栈。la 把 stack_top 的地址装入 sp，
 * 然后 call kernel_entry。call 会把返回地址保存在 ra 中。
 */
_start:
    la sp, stack_top
    call kernel_entry

/*
 * kernel_entry 如果返回，裸机环境中没有 exit() 可以调用。
 * wfi（wait for interrupt）让 hart 等待；如果它继续执行，就用 j 回到
 * halt_loop。这里是一个不会意外跑进后续数据的安全停留点。
 */
halt_loop:
    wfi
    j halt_loop

/*
 * 先打印公共开场信息，再调用构建时选中的 scenario。
 *
 * uart_puts 的第一个参数是字符串地址，按 RV64 calling convention 放在 a0。
 * la 只装入 msg_start 的地址，不读取字符串内容。
 */
kernel_entry:
    la a0, msg_start
    call uart_puts

/*
 * 这些条件在编译前由 C preprocessor 处理。最终 ELF 中的 kernel_entry
 * 只保留与 SCENARIO_ID 对应的 call，而不是在运行时逐个比较。
 */
#if SCENARIO_ID == 0
    call scenario_good
#elif SCENARIO_ID == 1
    call scenario_bad_pointer
#elif SCENARIO_ID == 2
    call scenario_bad_jump
#elif SCENARIO_ID == 3
    call scenario_illegal_instruction
#else
    call scenario_bad_pointer
#endif
    ret

/*
 * 正常基线：只打印信息并返回。
 * 如果这个场景也不能运行，应先检查工具链、QEMU 或 GDB 连接。
 */
scenario_good:
    la a0, msg_good
    call uart_puts
    ret

/*
 * 场景 1：合法的 load 指令使用了可疑地址0xdeadbeef。
 *
 * uart_puts 会使用并改写 caller-saved registers，所以先完成打印，
 * 再用 li 把固定常数写入 a0。
 */
scenario_bad_pointer:
    la a0, msg_bad_pointer
    call uart_puts
    li a0, 0xdeadbeef

/*
 * 建议在这里设置 breakpoint。
 *
 * ld a1, 0(a0) 的含义：
 *   effective address = a0 + 0
 *   从该地址读取 8 bytes
 *   把结果写入 a1
 *
 * 执行前应看到 a0 == 0xdeadbeef。a0 是地址来源，a1 是结果目标。
 */
bad_pointer_fault_site:
    ld a1, 0(a0)

    /*
     * 某些机器/映射下坏地址访问的外在表现可能不同。如果 load 居然返回，
     * 先打印提示，再用明确的非法位模式强制产生 exception。
     */
    la a0, msg_pointer_continued
    call uart_puts
    .word 0xffffffff

/*
 * 场景 2：通过寄存器执行 indirect jump。
 *
 * la 把 unexpected_landing 的地址放进 t0。此处的目标地址本身有效，
 * 但实验把它视为“出乎程序意图的落点”，用于观察控制流错误。
 */
scenario_bad_jump:
    la a0, msg_bad_jump
    call uart_puts
    la t0, unexpected_landing

/*
 * 建议先在 bad_jump_source 停下。
 *
 * jr t0 不直接写出目标 label；CPU 从 t0 取得下一条指令地址。
 * 因此调试这条指令时应同时检查 pc 和 t0。
 */
bad_jump_source:
    jr t0

/*
 * 这是有效代码，所以跳到这里不一定产生 exception。
 * 在此设置第二个 breakpoint，可以证明 jr t0 实际选择了这个落点。
 */
unexpected_landing:
    la a0, msg_unexpected_landing
    call uart_puts
    ret

/*
 * 场景 3：执行非法指令位模式。
 */
scenario_illegal_instruction:
    la a0, msg_illegal
    call uart_puts

/*
 * .word 是 assembler directive：它把 32-bit 值 0xffffffff 原样放进
 * .text。CPU 的 pc 到达这里时，会尝试把这四个 bytes 当作指令译码；
 * 该位模式在本实验 ISA 配置下不是合法指令，因此产生 illegal-instruction
 * exception。
 *
 * 本实验没有安装完整 trap handler，主要观察点是执行这条指令之前。
 */
illegal_instruction_site:
    .word 0xffffffff
    ret

/*
 * uart_puts(a0 = zero-terminated string address)
 *
 * 寄存器分工：
 *   t0  当前字符的地址
 *   t1  当前字符
 *   t2  UART register 地址
 *   t3  UART line-status register 的读取结果
 *
 * 这是 polling 输出：不断读取 UART0 + 5，直到 bit 0x20 表示 transmit
 * holding register 已经 ready，再把一个 byte 写到 UART0 + 0。
 */
uart_puts:
    mv t0, a0

/*
 * GNU assembler 的数字 local label 可以重复使用：
 *   1b = 向后寻找最近的 1:
 *   2b = 向后寻找最近的 2:
 *   3f = 向前寻找最近的 3:
 */
1:
    /* lbu 读取一个 unsigned byte；zero byte 表示字符串结束。 */
    lbu t1, 0(t0)
    beqz t1, 3f

2:
    /* UART line-status register 位于 base + 5。 */
    li t2, UART0 + 5
    lbu t3, 0(t2)
    andi t3, t3, 0x20

    /* 如果 transmit holding register 尚未 ready，就继续 polling。 */
    beqz t3, 2b

    /* ready 后，把当前字符的低 8 bits 写入 UART transmit register。 */
    li t2, UART0
    sb t1, 0(t2)

    /* 移动到下一个字符，然后回到 1:。 */
    addi t0, t0, 1
    j 1b

3:
    ret

/*
 * .rodata 保存只读数据。.asciz 会自动在字符串末尾添加 zero byte，
 * uart_puts 正是通过这个 zero byte 判断字符串结束。
 */
.section .rodata
msg_start:
    .asciz "Lab 13 kernel_entry reached\n"
msg_good:
    .asciz "Scenario good: no exception-style failure\n"
msg_bad_pointer:
    .asciz "Scenario bad-pointer: a0 will hold 0xdeadbeef before the load\n"
msg_pointer_continued:
    .asciz "The bad pointer load returned; illegal instruction will force a trap\n"
msg_bad_jump:
    .asciz "Scenario bad-jump: control will jump through t0\n"
msg_unexpected_landing:
    .asciz "Unexpected landing reached; inspect pc and the jump source\n"
msg_illegal:
    .asciz "Scenario illegal-instruction: next word is intentionally illegal\n"

/*
 * .bss 保存运行时需要的 zero-initialized space。
 *
 * .align 4 在 GNU RISC-V assembler 中按 2^4 = 16 bytes 对齐。
 * .skip 4096 预留 4096 bytes，但不需要在 ELF 中存储 4096 个显式零字节。
 * stack_top 位于预留区域末端；RISC-V 栈通常从高地址向低地址增长。
 */
.section .bss
.align 4
stack:
    .skip 4096
stack_top:
