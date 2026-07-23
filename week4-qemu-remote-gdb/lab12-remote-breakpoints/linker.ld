/* 最终输出文件使用 RISC-V 架构。 */
OUTPUT_ARCH(riscv)

/*
 * 把 _start 的最终地址写入 ELF header，作为 entry point。
 * ENTRY 只选择入口符号，不负责决定 _start 被放在哪个地址。
 */
ENTRY(_start)

/* 描述最终 ELF 中各个输出 section 的地址和排列顺序。 */
SECTIONS
{
  /*
   * “.” 是 linker 的 location counter，表示当前输出地址。
   * 后面的第一个输出 section 从 0x80000000 开始布局。
   */
  . = 0x80000000;

  /*
   * .text 保存 CPU 要执行的机器指令。
   *
   * KEEP(*(.text.start))
   *   从所有输入 .o 文件中收集 .text.start，并防止链接垃圾回收删除它。
   *   start.S 把 _start 放在这里，所以启动代码位于最终 .text 的最前面。
   *
   * *(.text*)
   *   收集所有输入文件中名称以 .text 开头的其余代码 section。
   */
  .text : {
    KEEP(*(.text.start))
    *(.text*)
  }

  /*
   * .rodata 保存运行时只读的数据，例如 UART 输出字符串。
   * .srodata 是工具链可能用于较小只读对象的输入 section。
   */
  .rodata : {
    *(.srodata*)
    *(.rodata*)
  }

  /*
   * .data 保存带有初始内容、运行时允许修改的数据。
   * 初始内容会存进 ELF，并在加载时复制到内存。
   * .sdata 是工具链可能用于较小可写对象的输入 section。
   */
  .data : {
    *(.sdata*)
    *(.data*)
  }

  /*
   * .bss 保存零初始化或未初始化的可写存储。本 Lab 的 checkpoint 状态
   * 和 start.S 中的栈位于这里。它们运行时占用内存，但 ELF 通常不保存
   * 一整块对应的零字节。
   *
   * .sbss   工具链可能用于较小零初始化对象的输入 section。
   * COMMON  兼容某些编译方式产生的 common 未初始化全局符号。
   * ALIGN   把 location counter 向上调整到下一个 16 字节边界。
   */
  .bss : {
    *(.sbss*)
    *(.bss*)
    *(COMMON)
    . = ALIGN(16);
  }
}
